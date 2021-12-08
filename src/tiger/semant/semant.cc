#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <iostream>
#include <map>

namespace absyn {

static int breakHierachy = 0;
static std::map<std::string, int> varCount;
static std::map<std::string, int> typCount;
static std::map<std::string, int> funCount;

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  int labelcount = 0;
  root_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if(entry && typeid(*entry)==typeid(env::VarEntry)){
    return (static_cast<env::VarEntry*>(entry))->ty_->ActualTy();
  }
  else{
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*(type->ActualTy()))!=typeid(type::RecordTy)){
    errormsg->Error(var_->pos_, "not a record type");
  }
  else{
    type::RecordTy *recordTy = static_cast<type::RecordTy*>(type->ActualTy());
    const std::list<type::Field*> tyFieldList = recordTy->fields_->GetList();
    for(const auto &tyField : tyFieldList){
      if(tyField->name_->Name()==sym_->Name()){
        return tyField->ty_;
      }
    }
    errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *type = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*(type->ActualTy()))!=typeid(type::ArrayTy)){
    errormsg->Error(var_->pos_, "array type required");
  }
  else{
    type::ArrayTy *arrayTy = static_cast<type::ArrayTy*>(type->ActualTy());
    type::Ty *indexTy = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if(!indexTy->ActualTy()->IsSameType(type::IntTy::Instance())){
      errormsg->Error(subscript_->pos_, "int type required");
    }
    else{
      return arrayTy->ty_;
    }
  }
  return nullptr;
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  // #ifdef CALLEXP
  // errormsg->Error(pos_, "call exp %s", func_->Name().data());
  // #endif
  env::EnvEntry *entry = venv->Look(func_);
  if(entry==nullptr || typeid(*entry)!=typeid(env::FunEntry)){
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
    return type::VoidTy::Instance();
  }
  else{
    env::FunEntry *funEntry = static_cast<env::FunEntry*>(entry);
    std::list<type::Ty*> tyList = funEntry->formals_->GetList();
    std::list<Exp*> expList = args_->GetList();
    // check argument list's length
    if(tyList.size()>expList.size()){
      errormsg->Error(pos_, "para type mismatch");
      return funEntry->result_->ActualTy();
    }
    else if(tyList.size()<expList.size()){
      errormsg->Error(pos_, "too many params in function g");
      return funEntry->result_->ActualTy();
    }
    else{
      // check arguments' type
      auto ty_it = tyList.begin();
      auto exp_it = expList.begin();
      for(;exp_it!=expList.end();++ty_it, ++exp_it){
        type::Ty *type = (*exp_it)->SemAnalyze(venv, tenv, labelcount, errormsg);
        if(!(*ty_it)->IsSameType(type)){
          errormsg->Error((*exp_it)->pos_, "para type mismatch");
          return funEntry->result_->ActualTy();
        }
      }
      return funEntry->result_->ActualTy();
    }
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left_ty = left_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  type::Ty *right_ty = right_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(oper_==absyn::PLUS_OP || oper_==absyn::MINUS_OP||oper_==absyn::TIMES_OP||oper_==absyn::DIVIDE_OP){
    if(!left_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(left_->pos_, "integer required");
    }
    else if(!right_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(right_->pos_, "integer required");
    }
    return type::IntTy::Instance();
  }
  else if(oper_==absyn::GT_OP || oper_==absyn::GE_OP || oper_==absyn::LT_OP || oper_==absyn::LE_OP){
    if(!left_ty->IsSameType(right_ty)){
      errormsg->Error(pos_, "same type required");
    }
    else if(!left_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(left_->pos_, "integer required");
    }
    else if(!right_ty->IsSameType(type::IntTy::Instance())){
      errormsg->Error(right_->pos_, "integer required");
    }
    return type::IntTy::Instance();
  }
  else{
    if(!left_ty->IsSameType(right_ty)){
      errormsg->Error(pos_, "same type required");
      return type::IntTy::Instance();
    }
    else{
      return type::IntTy::Instance();
    }
  }
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  #ifdef DEBUG
  errormsg->Error(pos_, "record exp starts");
  #endif
  type::Ty *type = tenv->Look(typ_);
  if(type==nullptr || typeid(*(type->ActualTy()))!=typeid(type::RecordTy)){
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return new type::RecordTy(new type::FieldList(nullptr));
  }
  else{
    #ifdef DEBUG
    errormsg->Error(pos_, "record type");
    #endif
    type::RecordTy *recordTy = static_cast<type::RecordTy*>(type->ActualTy());
    std::list<type::Field*> fieldList = recordTy->fields_->GetList();
    #ifdef DEBUG
    errormsg->Error(pos_, "1 error");
    #endif
    std::list<EField*> efieldList =  fields_->GetList();
    #ifdef DEBUG
    errormsg->Error(pos_, "2 error");
    #endif
    if(fieldList.size()!=efieldList.size()){
      errormsg->Error(pos_, "undefined record type");
      return recordTy;
    }
    else{
      #ifdef DEBUG
      errormsg->Error(pos_, "record field size matches");
      #endif
      auto field = fieldList.begin();
      auto efield = efieldList.begin();
      for(;field!=fieldList.end();++field, ++efield){
        // name unmatched
        if((*field)->name_->Name()!=(*efield)->name_->Name()){
          errormsg->Error(pos_, "field name unmatched");
          return recordTy;
        }
        if(!(*field)->ty_->IsSameType((*efield)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg))){
          errormsg->Error(pos_, "field type unmatched");
          return recordTy;
        }
      }
      return recordTy;
    }
  }
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp*> expList = seq_->GetList();
  type::Ty *type;
  for(const auto &exp : expList){
    type = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  return type;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *varType = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *expType = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*var_)==typeid(absyn::SimpleVar)){
    sym::Symbol *sym = (static_cast<SimpleVar*>(var_))->sym_;
    env::EnvEntry *entry = venv->Look(sym);
    env::VarEntry *varEntry = static_cast<env::VarEntry*>(entry);
    if(varEntry->readonly_){
      errormsg->Error(var_->pos_, "loop variable can't be assigned");
      return type::VoidTy::Instance();
    }
  }
  if(varType->IsSameType(expType)){
    return type::VoidTy::Instance();
  }
  else{
    errormsg->Error(pos_, "unmatched assign exp");
    return type::VoidTy::Instance();
  }
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(!test_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::IntTy::Instance())){
    errormsg->Error(test_->pos_, "test exp type should be int");
    return type::VoidTy::Instance();
  }
  else{
    // if then
    if(elsee_==nullptr){
      if(!then_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::VoidTy::Instance())){
        errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
        return type::VoidTy::Instance();
      }
      else{
        return type::VoidTy::Instance();
      }
    }
    // if then else
    else{
      type::Ty *thenTy = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
      type::Ty *elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!thenTy->IsSameType(elseTy)){
        errormsg->Error(pos_, "then exp and else exp type mismatch");
        return type::VoidTy::Instance();
      }
      else{
        if(typeid(*(thenTy->ActualTy()))==typeid(type::NilTy)){
          return elseTy;
        }
        else{
          return thenTy;
        }
      }
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(!test_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::IntTy::Instance())){
    errormsg->Error(test_->pos_, "test exp type should be int");
    return type::VoidTy::Instance();
  }
  breakHierachy++;
  type::Ty *type = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(!type->IsSameType(type::VoidTy::Instance())){
    errormsg->Error(body_->pos_, "while body must produce no value");
  }
  breakHierachy--;
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(!lo_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::IntTy::Instance())){
    errormsg->Error(lo_->pos_, "for exp's range type is not integer");
  }
  if(!hi_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::IntTy::Instance())){
    errormsg->Error(hi_->pos_, "for exp's range type is not integer");
  }
  venv->BeginScope();
  breakHierachy++;
  venv->Enter(var_, new env::VarEntry(type::IntTy::Instance(), true));
  if(!body_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::VoidTy::Instance())){
    errormsg->Error(body_->pos_, "for exp body type should be void");
  }
  venv->EndScope();
  breakHierachy--;
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(breakHierachy<=0){
    errormsg->Error(pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  #ifdef DEBUG
  std::cout << "let exp starts" << std::endl;
  #endif
  venv->BeginScope();
  tenv->BeginScope();
  std::map<std::string, int> varCountTmp = varCount;
  std::map<std::string, int> typCountTmp = typCount;
  std::map<std::string, int> funCountTmp = funCount;
  varCount.clear();
  typCount.clear();
  funCount.clear();
  std::list<Dec*> decList = decs_->GetList();
  for(const auto &dec : decList){
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  int tmpHie = breakHierachy;
  breakHierachy = 0;
  type::Ty *type = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  breakHierachy = tmpHie;
  #ifdef DEBUG
  std::cout << "let exp ends" << std::endl;
  #endif
  venv->EndScope();
  tenv->BeginScope();
  varCount = varCountTmp;
  typCount = typCountTmp;
  funCount = funCountTmp;
  return type;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  #ifdef DEBUG
  errormsg->Error(pos_, "Array exp starts");
  #endif
  type::Ty *type = tenv->Look(typ_);
  if(type==nullptr||typeid(*(type->ActualTy()))!=typeid(type::ArrayTy)){
    errormsg->Error(pos_, "undefined array type");
  }
  else if(!size_->SemAnalyze(venv, tenv, labelcount, errormsg)->IsSameType(type::IntTy::Instance())){
    errormsg->Error(size_->pos_, "size must be a integer");
  }
  else{
    type::ArrayTy *arrayTy = static_cast<type::ArrayTy*>(type->ActualTy());
    #ifdef DEBUG
    errormsg->Error(pos_, "Array exp ends");
    #endif
    if(!arrayTy->ty_->IsSameType(init_->SemAnalyze(venv, tenv, labelcount, errormsg))){
      errormsg->Error(init_->pos_, "type mismatch");
    }
  }
  #ifdef DEBUG
  errormsg->Error(pos_, "Array exp ends");
  #endif
  return type->ActualTy();
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec*> funDecList  = functions_->GetList();
  // first traversal
  for(const auto &funDec : funDecList){
    funCount[funDec->name_->Name()]++;
  }
  for(const auto &pair_ : funCount){
    if(pair_.second>1){
      errormsg->Error(pos_, "two functions have the same name");
      return;
    }
  }
  for(const auto &funDec : funDecList){
    if(funDec->result_!=nullptr){
      type::Ty *result_ty = tenv->Look(funDec->result_);
      if(result_ty==nullptr){
        errormsg->Error(pos_, "return value type undefined");
      }
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      venv->Enter(funDec->name_, new env::FunEntry(formals, result_ty));
    }
    else{
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      venv->Enter(funDec->name_, new env::FunEntry(formals, type::VoidTy::Instance()));
    }
  }
  // second traversal
  for(const auto &funDec : funDecList){
    if(funDec->result_!=nullptr){
      venv->BeginScope();
      int tmpHie = breakHierachy;
      breakHierachy = 0;
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      auto formal_it = formals->GetList().begin();
      auto param_it = funDec->params_->GetList().begin();
      for(;param_it != funDec->params_->GetList().end();formal_it++, param_it++){
        venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
      }
      type::Ty *result_ty = tenv->Look(funDec->result_);
      type::Ty *ty = funDec->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!result_ty->IsSameType(ty)){
        errormsg->Error(pos_, "return type unmatched");
      }
      breakHierachy = tmpHie;
      venv->EndScope();
    }
    else{
      venv->BeginScope();
      int tmpHie = breakHierachy;
      breakHierachy = 0;
      type::TyList *formals = funDec->params_->MakeFormalTyList(tenv, errormsg);
      auto formal_it = formals->GetList().begin();
      auto param_it = funDec->params_->GetList().begin();
      for(;param_it != funDec->params_->GetList().end();formal_it++, param_it++){
        venv->Enter((*param_it)->name_, new env::VarEntry(*formal_it));
      }
      type::Ty *result_ty = tenv->Look(funDec->result_);
      type::Ty *ty = funDec->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!ty->IsSameType(type::VoidTy::Instance())){
        errormsg->Error(funDec->body_->pos_, "procedure returns value");
      }
      breakHierachy = tmpHie;
      venv->EndScope();
    }
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  varCount[var_->Name()]++;
  if(varCount[var_->Name()]>1){
    errormsg->Error(pos_, "two variables have the same name");
    return;
  }
  type::Ty *init_ty = init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typ_==nullptr){
    if(typeid(*(init_ty->ActualTy()))==typeid(type::NilTy)){
      errormsg->Error(init_->pos_, "init should not be nil without type specified");
      venv->Enter(var_, new env::VarEntry(init_ty));
    }
    else{
      venv->Enter(var_, new env::VarEntry(init_ty));
    }
  }
  else{
    type::Ty *target_ty = tenv->Look(typ_);
    if(target_ty){
      if(target_ty->IsSameType(init_ty)){
        venv->Enter(var_, new env::VarEntry(target_ty));
      }
      else{
        errormsg->Error(pos_, "type mismatch");
        venv->Enter(var_, new env::VarEntry(target_ty));
      }
    }
    else{
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      venv->Enter(var_, new env::VarEntry(type::IntTy::Instance()));
    }
  }
  #ifdef DEBUG
  errormsg->Error(pos_, "var dec ends");
  #endif
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  #ifdef DEBUG
  errormsg->Error(pos_, "type dec starts");
  #endif
  const std::list<NameAndTy*> nameAndTyList = types_->GetList();
  // first traversal
  for(const auto &nameAndTy : nameAndTyList){
    typCount[nameAndTy->name_->Name()]++;
  }
  for(const auto &pair_ : typCount){
    if(pair_.second>1){
      errormsg->Error(pos_, "two types have the same name");
      return;
    }
  }
  for(const auto &nameAndTy : nameAndTyList){
    tenv->Enter(nameAndTy->name_, new type::NameTy(nameAndTy->name_, nullptr));
  }
  // second traversal
  for(const auto &nameAndTy : nameAndTyList){
    type::NameTy *type = static_cast<type::NameTy*>(tenv->Look(nameAndTy->name_));
    type->ty_ = nameAndTy->ty_->SemAnalyze(tenv, errormsg);
    tenv->Set(nameAndTy->name_, type);
  }
  // third traversal to check if there exists a cycle
  for(const auto &nameAndTy : nameAndTyList){
    // unfinished here
    type::Ty *origin = tenv->Look(nameAndTy->name_);
    type::Ty *type = origin;
    type::Ty *fast = origin;
    type::Ty *slow = origin;
    bool cycle = false;
    while(true){
      if(typeid(*fast)==typeid(type::NameTy)){
        type::Ty *fastNext = static_cast<type::NameTy*>(fast)->ty_;
        if(typeid(*fastNext)==typeid(type::NameTy)){
          type::Ty *fastNextNext = static_cast<type::NameTy*>(fastNext)->ty_;
          fast = fastNext;
          slow = static_cast<type::NameTy*>(slow)->ty_;
          if(fast==slow){
            cycle = true;
            break;
          }
        }
        else{
          break;
        }
      }
      else{
        break;
      }
    }
    if(cycle){
      errormsg->Error(pos_, "illegal type cycle");
      break;
    }
  }
  #ifdef DEBUG
  errormsg->Error(pos_, "type dec ends");
  #endif
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *type = tenv->Look(name_);
  if(type==nullptr){
    errormsg->Error(pos_, "undefined type %s", name_->Name().data());
    return new type::NameTy(name_, nullptr);
  }
  else{
    return type;
  }
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<absyn::Field*> absynFieldList = record_->GetList();
  type::FieldList *tyFieldList = new type::FieldList();
  for(const auto &absynField : absynFieldList){
    type::Ty *type = tenv->Look(absynField->typ_);
    if(type==nullptr){
      errormsg->Error(pos_, "undefined type %s", absynField->typ_->Name().data());
    }
    else{
      tyFieldList->Append(new type::Field(absynField->name_, type));
    }
  }
  return new type::RecordTy(tyFieldList);
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *type = tenv->Look(array_);
  if(type==nullptr){
    errormsg->Error(pos_, "undefined type %s", array_->Name().data());
  }
  else{
    return new type::ArrayTy(type);
  }
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
