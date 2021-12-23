#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <stdexcept>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

static int loop_variable = 0;

namespace tr {

tr::Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Access *fAccess = level->frame_->allocLocal(escape);
  tr::Access *tAccess = new tr::Access(level, fAccess);
  return tAccess;
}

Level* NewLevel(Level *parent, temp::Label *name, const std::list<bool>& escapes) {
  std::list<bool> tmp_escapes(escapes);
  tmp_escapes.push_front(true);
  frame::Frame *frame = frame::X64Frame::NewFrame(name, tmp_escapes);
  Level *level = new Level(frame, parent);
  return level;
}

class Cx {
public:
  temp::Label **trues_;
  temp::Label **falses_;
  tree::Stm *stm_;

  Cx(temp::Label **trues, temp::Label **falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() const = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() const = 0;
  [[nodiscard]] virtual Cx UnCx() const = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() const override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx() const override {
    /* TODO: Put your lab5 code here */
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    tree::CjumpStm *stm = new tree::CjumpStm(tree::NE_OP, exp_, new tree::ConstExp(0), t, f);
    return Cx(&(stm->true_label_), &(stm->false_label_), stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() const override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx() const override {
    /* TODO: Put your lab5 code here */
    throw std::runtime_error("unexpected situation");
    return Cx(nullptr, nullptr, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(temp::Label** trues, temp::Label** falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() const override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    *(cx_.trues_) = t;
    *(cx_.falses_) = f;
    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(1)),
      new tree::EseqExp(
        cx_.stm_,
        new tree::EseqExp(
          new tree::LabelStm(f),
          new tree::EseqExp(
            new tree::MoveStm(new tree::TempExp(r), new tree::ConstExp(0)),
            new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(r))
          )
        )
      )
    );
  }
  [[nodiscard]] tree::Stm *UnNx() const override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx() const override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  std::list<bool> escapes;
  temp::Label *mainLabel = temp::LabelFactory::NamedLabel("tigermain");
  frame::Frame *frame = frame::X64Frame::NewFrame(mainLabel, escapes);
  main_level_ = std::make_unique<Level>(frame, nullptr);
  FillBaseTEnv();
  FillBaseVEnv();
  tr::ExpAndTy *expAndTy = absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(), mainLabel, errormsg_.get());
  tree::Stm *stm = expAndTy->exp_->UnNx();
  frags->PushBack(new frame::ProcFrag(stm, frame));
}

tr::Exp* nilExp() {
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp* simpleVar(tr::Access *access, tr::Level *level) {
  tree::Exp *frame = new tree::TempExp(reg_manager->FramePointer());
  while(level != access->level_) {
    frame = new tree::MemExp(
      new tree::BinopExp(
        tree::PLUS_OP, 
        frame, 
        new tree::ConstExp(-reg_manager->WordSize())
      )
    );
    level = level->parent_;
  }
  frame = access->access_->ToExp(frame);
  return new tr::ExExp(frame);
}

tr::Exp* fieldVar(tr::Exp* exp, int offset) {
  return new tr::ExExp(
    new tree::MemExp(
      new tree::BinopExp(
        tree::PLUS_OP, 
        exp->UnEx(), 
        new tree::ConstExp(offset * reg_manager->WordSize())
      )
    )
  );
}

tr::Exp* subscriptVar(tr::Exp* var, tr::Exp* subscript) {
  return new tr::ExExp(
    new tree::MemExp(
      new tree::BinopExp(
        tree::PLUS_OP,
        var->UnEx(),
        new tree::BinopExp(
          tree::MUL_OP,
          subscript->UnEx(),
          new tree::ConstExp(reg_manager->WordSize())
        )
      )
    )
  );
}

tr::Exp* string(std::string str) {
  temp::Label *label = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(label, str));
  return new tr::ExExp(new tree::NameExp(label));
}

tr::Exp* callExp(tr::Level *caller, tr::Level* callee, temp::Label* label, std::list<tr::Exp*> args) {
  tree::ExpList* treeExpList = new tree::ExpList();
  for(const auto& arg : args){
    treeExpList->Append(arg->UnEx());
  }
  tr::Level* directUpper = caller;
  tree::Exp* staticlink = new tree::TempExp(caller->frame_->FramePointer());
  while(callee->parent_ != directUpper) {
    staticlink = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, staticlink, new tree::ConstExp(-reg_manager->WordSize())));
    directUpper = callee->parent_;
  }
  if(directUpper==nullptr) {
    return new tr::ExExp(frame::X64Frame::externalCall(label->Name(), treeExpList));
  }
  else{
    treeExpList->Insert(staticlink);
    return new tr::ExExp(new tree::CallExp(new tree::NameExp(label), treeExpList));
  }
}

tr::Exp* calculateExp(tr::Exp *left, tr::Exp *right, absyn::Oper oper) {
  tr::Exp* trExp;
  switch(oper) {
    case absyn::PLUS_OP:
      trExp = new tr::ExExp(new tree::BinopExp(tree::PLUS_OP, left->UnEx(), right->UnEx()));
      break;
    case absyn::MINUS_OP:
      trExp = new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, left->UnEx(), right->UnEx()));
      break;
    case absyn::TIMES_OP:
      trExp = new tr::ExExp(new tree::BinopExp(tree::MUL_OP, left->UnEx(), right->UnEx()));
      break;
    case absyn::DIVIDE_OP:
      trExp = new tr::ExExp(new tree::BinopExp(tree::DIV_OP, left->UnEx(), right->UnEx()));
      break;
    default:
      throw std::runtime_error("gee2!");
      break;
  }
  return trExp;
}

tr::Exp* stringEqual(tr::Exp *left, tr::Exp *right) {
  tree::ExpList *treeExpList = new tree::ExpList();
  treeExpList->Append(left->UnEx());
  treeExpList->Append(right->UnEx());
  return new tr::ExExp(frame::X64Frame::externalCall(std::string("string_equal"), treeExpList));
}

tr::Exp* relationExp(tr::Exp *left, tr::Exp *right, absyn::Oper oper){
  tree::CjumpStm *stm;
  switch(oper) {
    case absyn::EQ_OP:
      stm = new tree::CjumpStm(tree::RelOp::EQ_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    case absyn::NEQ_OP:
      stm = new tree::CjumpStm(tree::RelOp::NE_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    case absyn::LT_OP:
      stm = new tree::CjumpStm(tree::RelOp::LT_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    case absyn::LE_OP:
      stm = new tree::CjumpStm(tree::RelOp::LE_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    case absyn::GT_OP:
      stm = new tree::CjumpStm(tree::RelOp::GT_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    case absyn::GE_OP:
      stm = new tree::CjumpStm(tree::RelOp::GE_OP, left->UnEx(), right->UnEx(), nullptr, nullptr);
      break;
    default:
      throw std::runtime_error("gee!");
      break;
  }
  return new tr::CxExp(&(stm->true_label_), &(stm->false_label_), stm);
}

tr::Exp* recordExp(const std::vector<tr::Exp*> &trExpList) {
  temp::Temp *ptr = temp::TempFactory::NewTemp();
  int count = trExpList.size();
  tree::Exp *allocExp = frame::X64Frame::externalCall(
    std::string("alloc_record"), 
    new tree::ExpList({new tree::ConstExp(reg_manager->WordSize() * count)})
  );
  tree::Stm *stm = new tree::MoveStm(
    new tree::TempExp(ptr),
    allocExp
  );
  for(int i=0;i<trExpList.size();++i){
    stm = new tree::SeqStm(
      stm,
      new tree::MoveStm(
        new tree::MemExp(
          new tree::BinopExp(
            tree::BinOp::PLUS_OP,
            new tree::TempExp(ptr),
            new tree::ConstExp(i * reg_manager->WordSize())
          )
        ) ,
        trExpList[i]->UnEx()
      )
    );
  }
  return new tr::ExExp(new tree::EseqExp(stm, new tree::TempExp(ptr)));
}

tr::Exp* arrayExp(tr::Exp *size, tr::Exp *init) {
  tree::ExpList *expList = new tree::ExpList();
  expList->Append(size->UnEx());
  expList->Append(init->UnEx());
  return new tr::ExExp(frame::X64Frame::externalCall(std::string("init_array"), expList));
}

tr::Exp* seqExp(const std::vector<tr::Exp*> &trExpArray) {
  if(trExpArray.size()==1) {
    return trExpArray[0];
  }
  else if(trExpArray.size()==2) {
    return new tr::ExExp(new tree::EseqExp(trExpArray[0]->UnNx(), trExpArray[1]->UnEx()));
  }
  else {
    tree::Stm *stm = trExpArray[0]->UnNx();
    for(int i=1;i < trExpArray.size()-1;++i){
      stm = new tree::SeqStm(stm, trExpArray[i]->UnNx());
    }
    tree::Exp *exp = new tree::EseqExp(stm, trExpArray[trExpArray.size()-1]->UnEx());
    return new tr::ExExp(exp);
  }
}

tr::Exp* assignExp(tr::Exp *var, tr::Exp *exp) {
  tree::Stm *mvExp = new tree::MoveStm(var->UnEx(), exp->UnEx());
  return new tr::NxExp(mvExp);
}

tr::Exp* ifExp(tr::Exp *test, tr::Exp *then, tr::Exp *elsee) {
  tr::Cx cx = test->UnCx();
  temp::Label *true_label = temp::LabelFactory::NewLabel();
  temp::Label *false_label = temp::LabelFactory::NewLabel();
  temp::Label *final_label = temp::LabelFactory::NewLabel();
  *(cx.trues_) = true_label;
  *(cx.falses_) = false_label;
  if(elsee) {
    temp::Temp *dst = temp::TempFactory::NewTemp();
    std::vector<temp::Label*>* jmp = new std::vector<temp::Label*>();
    jmp->push_back(final_label);
    return new tr::ExExp(
      new tree::EseqExp(cx.stm_, 
        new tree::EseqExp(new tree::LabelStm(true_label), 
          new tree::EseqExp(new tree::MoveStm(new tree::TempExp(dst), then->UnEx()),
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(final_label), jmp),
              new tree::EseqExp(new tree::LabelStm(false_label),
                new tree::EseqExp(new tree::MoveStm(new tree::TempExp(dst), elsee->UnEx()),
                  new tree::EseqExp(new tree::JumpStm(new tree::NameExp(final_label), jmp),
                    new tree::EseqExp(new tree::LabelStm(final_label), new tree::TempExp(dst))))))))));
  }
  else{
    return new tr::NxExp(
      new tree::SeqStm(cx.stm_,
        new tree::SeqStm(new tree::LabelStm(true_label),
          new tree::SeqStm(then->UnNx(), new tree::LabelStm(false_label)))));
  }
}

tr::Exp* whileExp(tr::Exp *test, tr::Exp *body, temp::Label *doneLabel) {
  tr::Cx cx = test->UnCx();
  temp::Label *testLabel = temp::LabelFactory::NewLabel();
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel();
  *(cx.trues_) = bodyLabel;
  *(cx.falses_) = doneLabel;
  std::vector<temp::Label*> *jmp = new std::vector<temp::Label*>();
  jmp->push_back(testLabel);
  return new tr::NxExp(
    new tree::SeqStm(new tree::LabelStm(testLabel),
      new tree::SeqStm(cx.stm_,
        new tree::SeqStm(new tree::LabelStm(bodyLabel),
          new tree::SeqStm(body->UnNx(),
            new tree::SeqStm(new tree::JumpStm(new tree::NameExp(testLabel), jmp),
              new tree::LabelStm(doneLabel)))))));
}

tr::Exp* breakExp(temp::Label *done) {
  std::vector<temp::Label*> *jmp = new std::vector<temp::Label*>();
  jmp->push_back(done);
  return new tr::NxExp(
    new tree::JumpStm(new tree::NameExp(done), jmp)
  );
}

void funcDec(tr::Exp *body, tr::Level *level) {
  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()), body->UnEx());
  tree::Stm *proc = level->frame_->procEntryExit1(stm);
  frags->PushBack(new frame::ProcFrag(proc, level->frame_));
}

} // namespace tr

namespace absyn {

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  type::Ty* type = ((env::VarEntry*)entry)->ty_;
  tr::Access* access = ((env::VarEntry*)entry)->access_;
  return new tr::ExpAndTy(tr::simpleVar(access, level), type);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *expAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  type::RecordTy *type = (type::RecordTy*)(expAndTy->ty_->ActualTy());
  const std::list<type::Field*> tyFieldList = type->fields_->GetList();
  int offset = 0;
  for(const auto &tyField : tyFieldList){
    if(tyField->name_->Name()==sym_->Name()) {
      return new tr::ExpAndTy(tr::fieldVar(expAndTy->exp_, offset), expAndTy->ty_);
    }
    ++offset;
  }
  errormsg->Error(pos_, "gee!");
  return new tr::ExpAndTy(nullptr, nullptr);
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *varExpAndTy = var_->Translate(venv, tenv, level, label, errormsg);
  type::ArrayTy *arrayType = (type::ArrayTy*)(varExpAndTy->ty_->ActualTy());
  tr::ExpAndTy *subscriptExpAndTy = subscript_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(tr::subscriptVar(varExpAndTy->exp_, subscriptExpAndTy->exp_), arrayType->ty_->ActualTy());
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(tr::nilExp(), type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)), type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(tr::string(str_), type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::FunEntry *funEntry = (env::FunEntry*)(venv->Look(func_));
  std::list<type::Ty*> tyList = funEntry->formals_->GetList();
  std::list<absyn::Exp*> absExpList = args_->GetList();
  tree::ExpList *treeExpList = new tree::ExpList();
  std::list<tr::Exp*> trExpList;
  treeExpList->Append(new tree::TempExp(reg_manager->FramePointer()));
  for(const auto& absExp : absExpList) {
    tr::ExpAndTy* expAndTy = absExp->Translate(venv, tenv, level, label, errormsg);
    trExpList.push_back(expAndTy->exp_);
  }
  return new tr::ExpAndTy(tr::callExp(level, funEntry->level_, func_, trExpList), funEntry->result_->ActualTy());
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy* left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy* right = right_->Translate(venv, tenv, level, label, errormsg);
  if(oper_==PLUS_OP || oper_==MINUS_OP || oper_==TIMES_OP || oper_==DIVIDE_OP) {
    return new tr::ExpAndTy(tr::calculateExp(left->exp_, right->exp_, oper_), type::IntTy::Instance());
  }
  else if(left->ty_->IsSameType(type::StringTy::Instance()) && right->ty_->IsSameType(type::StringTy::Instance()) && oper_==EQ_OP) {
    return new tr::ExpAndTy(tr::stringEqual(left->exp_, right->exp_), type::IntTy::Instance());
  }
  else{
    return new tr::ExpAndTy(tr::relationExp(left->exp_, right->exp_, oper_), type::IntTy::Instance());
  }
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *type = tenv->Look(typ_)->ActualTy();
  std::list<absyn::EField*> efieldList = fields_->GetList();
  std::vector<tr::Exp*> trExpList;
  for(const auto& efield : efieldList) {
    tr::ExpAndTy *expAndTy = efield->exp_->Translate(venv, tenv, level, label, errormsg);
    trExpList.push_back(expAndTy->exp_);
  }
  return new tr::ExpAndTy(tr::recordExp(trExpList), type);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::Exp*> absExpList = seq_->GetList();
  std::vector<tr::Exp*> trExpArray;
  tr::ExpAndTy* expAndTy;
  for(const auto& absExp : absExpList) {
    expAndTy = absExp->Translate(venv, tenv, level, label, errormsg);
    trExpArray.push_back(expAndTy->exp_);
  }
  return new tr::ExpAndTy(tr::seqExp(trExpArray), expAndTy->ty_);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(tr::assignExp(var->exp_, exp->exp_), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);
  if(elsee_) {
    tr::ExpAndTy *elsee = elsee_->Translate(venv, tenv, level, label, errormsg);
    type::Ty *type = typeid(*(then->ty_->ActualTy()))==typeid(type::NilTy) ? elsee->ty_ : then->ty_;
    return new tr::ExpAndTy(tr::ifExp(test->exp_, then->exp_, elsee->exp_), type);
  }
  else{
    return new tr::ExpAndTy(tr::ifExp(test->exp_, then->exp_, nullptr), type::VoidTy::Instance());
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, done, errormsg);
  return new tr::ExpAndTy(tr::whileExp(test->exp_, body->exp_, done), body->ty_);
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  loop_variable++;
  absyn::DecList *decList = new absyn::DecList();
  absyn::ExpList *expList = new absyn::ExpList();
  std::string loop_str = "loop_variable_" + std::to_string(loop_variable);
  sym::Symbol *symbol = sym::Symbol::UniqueSymbol(loop_str);
  absyn::VarDec *loopVarDec = new VarDec(0, var_, nullptr, lo_);
  loopVarDec->escape_ = escape_;
  decList->Prepend(new VarDec(0, symbol, nullptr, hi_));
  decList->Prepend(loopVarDec);
  expList->Prepend(new AssignExp(
    body_->pos_, 
    new SimpleVar(body_->pos_, var_), 
    new OpExp(
      0, 
      absyn::Oper::PLUS_OP, 
      new VarExp(pos_, new SimpleVar(pos_, var_)), 
      new IntExp(0, 1))));
  expList->Prepend(body_);
  absyn::Exp *transformed = new LetExp(
    pos_, 
    decList, 
    new WhileExp(
      body_->pos_,
      new OpExp(
        0, 
        absyn::Oper::LE_OP, 
        new VarExp(pos_, new SimpleVar(pos_, var_)), 
        new VarExp(pos_, new SimpleVar(pos_, symbol))
      ), 
      new absyn::SeqExp(body_->pos_, expList)
      )
    );
  return transformed->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(tr::breakExp(label), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Dec*> decList = decs_->GetList();
  std::vector<tr::Exp*> trExpArray;
  for(auto &dec : decList) {
    tr::Exp *trExp = dec->Translate(venv, tenv, level, label, errormsg);
    trExpArray.push_back(trExp);
  }
  tr::ExpAndTy *expAndTy = body_->Translate(venv, tenv, level, label, errormsg);
  trExpArray.push_back(expAndTy->exp_);
  return new tr::ExpAndTy(tr::seqExp(trExpArray), expAndTy->ty_);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *type = tenv->Look(typ_);
  tr::ExpAndTy *size = size_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  return new tr::ExpAndTy(tr::arrayExp(size->exp_, init->exp_), type);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::NxExp(new tree::ExpStm(new tree::ConstExp(0))), type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec*> funDecList = functions_->GetList();
  for(const auto& funDec : funDecList) {
    temp::Label *nameLabel = temp::LabelFactory::NamedLabel(funDec->name_->Name());
    std::list<absyn::Field*> fieldList = funDec->params_->GetList();
    std::list<bool> escapeList;
    for(const auto &field : fieldList) {
      escapeList.push_back(field->escape_);
    }
    tr::Level *newLevel = tr::NewLevel(level, nameLabel, escapeList);
    if(funDec->result_!=nullptr) {
      type::Ty *result_ty = tenv->Look(funDec->result_);
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      venv->Enter(funDec->name_, new env::FunEntry(newLevel, nameLabel, formals, result_ty));
    }
    else{
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      venv->Enter(funDec->name_, new env::FunEntry(newLevel, nameLabel, formals, type::VoidTy::Instance()));
    }
  }
  for(const auto& funDec : funDecList) {
    if(funDec->result_!=nullptr) {
      venv->BeginScope();
      env::FunEntry *entry = (env::FunEntry*)(venv->Look(funDec->name_));
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      // need to escape the first arg - static link
      std::list<tr::Access*> accessList = entry->level_->GetFormals();
      auto access_it = accessList.begin();
      access_it++;
      auto formal_it = formals->GetList().begin();
      auto param_it = funDec->params_->GetList().begin();
      for(;param_it != funDec->params_->GetList().end();formal_it++, param_it++, access_it++){
        venv->Enter((*param_it)->name_, new env::VarEntry(*access_it, *formal_it));
      }
      type::Ty *result_ty = tenv->Look(funDec->result_);
      tr::ExpAndTy *expAndTy = funDec->body_->Translate(venv, tenv, entry->level_, label, errormsg);
      venv->EndScope();
      tr::funcDec(expAndTy->exp_, entry->level_);
    }
    else{
      venv->BeginScope();
      env::FunEntry *entry = (env::FunEntry*)(venv->Look(funDec->name_));
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      std::list<tr::Access*> accessList = entry->level_->GetFormals();
      auto access_it = accessList.begin();
      access_it++;
      auto formal_it = formals->GetList().begin();
      auto param_it = funDec->params_->GetList().begin();
      for(;param_it != funDec->params_->GetList().end();formal_it++, param_it++, access_it++){
        venv->Enter((*param_it)->name_, new env::VarEntry(*access_it, *formal_it));
      }
      tr::ExpAndTy *expAndTy = funDec->body_->Translate(venv, tenv, entry->level_, label, errormsg);
      venv->EndScope();
      tr::funcDec(expAndTy->exp_, entry->level_);
    }
  }
  return tr::nilExp();
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  tr::Access *access = tr::Access::AllocLocal(level, escape_);
  venv->Enter(var_, new env::VarEntry(access, init->ty_, false));
  return new tr::NxExp(new tree::MoveStm(tr::simpleVar(access, level)->UnEx(), init->exp_->UnEx()));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::NameAndTy*> nameAndTyList = types_->GetList();
  for(const auto &nameAndTy : nameAndTyList) {
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  for(const auto &nameAndTy : nameAndTyList) {
    type::NameTy *type = (type::NameTy*)(tenv->Look(nameAndTy->name_));
    type->ty_ = nameAndTy->ty_->Translate(tenv, errormsg);
    tenv->Set(nameAndTy->name_, type);
  }
  return tr::nilExp();
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::NameTy(name_, tenv->Look(name_));
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::Field*> absynFieldList = record_->GetList();
  type::FieldList *tyFieldList = new type::FieldList();
  for(const auto &absynField : absynFieldList){
    type::Ty *type = tenv->Look(absynField->typ_);
    tyFieldList->Append(new type::Field(absynField->name_, type));
  }
  return new type::RecordTy(tyFieldList);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn
