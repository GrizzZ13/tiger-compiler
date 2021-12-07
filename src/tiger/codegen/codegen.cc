#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;

} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  fs_ = frame_->name_->Name() + "_framesize";
  assem::InstrList *instr_list = new assem::InstrList();
  assem_instr_ = std::make_unique<AssemInstr>(instr_list);
  std::list<tree::Stm *> stmList = traces_->GetStmList()->GetList();
  for (auto &stm : stmList) {
    stm->Munch(*instr_list, fs_);
  }
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(label_->Name(), label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::OperInstr("jmp `j0", nullptr, nullptr,
                                         new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string jxx;
  switch (op_) {
  case tree::RelOp::EQ_OP:
    jxx = "je";
    break;
  case tree::RelOp::NE_OP:
    jxx = "jne";
    break;
  case tree::RelOp::LT_OP:
    jxx = "jl";
    break;
  case tree::RelOp::LE_OP:
    jxx = "jle";
    break;
  case tree::RelOp::GT_OP:
    jxx = "jg";
    break;
  case tree::RelOp::GE_OP:
    jxx = "jge";
    break;
  default:
    assert(0);
  }
  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);
  temp::TempList *tempList = new temp::TempList();
  tempList->Append(left);
  tempList->Append(right);
  std::vector<temp::Label *> *labels = new std::vector<temp::Label *>();
  labels->push_back(true_label_);
  labels->push_back(false_label_);
  assem::Targets *targets = new assem::Targets(labels);
  instr_list.Append(
      new assem::OperInstr("cmpq `s1, `s0", nullptr, tempList, nullptr));
  instr_list.Append(
      new assem::OperInstr(jxx + " `j0", nullptr, nullptr, targets));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*dst_) == typeid(MemExp)) {
    MemExp *memDst = (MemExp *)dst_;
    if (typeid(*(memDst->exp_)) == typeid(BinopExp) &&
        ((BinopExp *)memDst->exp_)->op_ == BinOp::PLUS_OP &&
        typeid(*(((BinopExp *)(memDst->exp_))->left_)) == typeid(ConstExp)) {
      Exp *exp1 = ((BinopExp *)memDst->exp_)->right_;
      Exp *exp2 = src_;
      int i = ((ConstExp *)((BinopExp *)memDst->exp_)->left_)->consti_;
      temp::Temp *temp1 = exp1->Munch(instr_list, fs);
      temp::Temp *temp2 = exp2->Munch(instr_list, fs);
      if(temp1==reg_manager->FramePointer()){
        //FIXME
        std::string movq = "movq `s0, (" + std::string(fs.data()) + "+" + std::to_string(i) + ")(`d0)";
        instr_list.Append(new assem::OperInstr(
          movq, new temp::TempList(reg_manager->StackPointer()), new temp::TempList(temp2), nullptr));
      }
      else{
        std::string movq = "movq `s0, " + std::to_string(i) + "(`d0)";
        instr_list.Append(new assem::OperInstr(
          movq, new temp::TempList(temp1), new temp::TempList(temp2), nullptr));
      }
    } else if (typeid(*(memDst->exp_)) == typeid(BinopExp) &&
               ((BinopExp *)memDst->exp_)->op_ == BinOp::PLUS_OP &&
               typeid(*(((BinopExp *)(memDst->exp_))->right_)) ==
                   typeid(ConstExp)) {
      Exp *exp1 = ((BinopExp *)memDst->exp_)->left_;
      Exp *exp2 = src_;
      int i = ((ConstExp *)((BinopExp *)memDst->exp_)->right_)->consti_;
      temp::Temp *temp1 = exp1->Munch(instr_list, fs);
      temp::Temp *temp2 = exp2->Munch(instr_list, fs);
      if(temp1==reg_manager->FramePointer()){
        //FIXME
        std::string movq = "movq `s0, (" + std::string(fs.data()) + "+" + std::to_string(i) + ")(`d0)";
        instr_list.Append(new assem::OperInstr(
          movq, new temp::TempList(reg_manager->StackPointer()), new temp::TempList(temp2), nullptr));
      }
      else{
        std::string movq = "movq `s0, " + std::to_string(i) + "(`d0)";
        instr_list.Append(new assem::OperInstr(
          movq, new temp::TempList(temp1), new temp::TempList(temp2), nullptr));
      }
    }
    // else if(typeid(*(memDst->exp_))==typeid(ConstExp)){
    //   temp::Temp *temp1 = memDst->exp_->Munch(instr_list, fs);
    //   temp::Temp *temp2 = src_->Munch(instr_list, fs);
    //   instr_list.Append(new assem::MoveInstr("movq `s0, (`d0)", new
    //   temp::TempList(temp1), new temp::TempList(temp2)));
    // }
    else if (typeid(*src_) == typeid(MemExp)) {
      temp::Temp *srcTemp = ((MemExp *)src_)->exp_->Munch(instr_list, fs);
      temp::Temp *dstTemp = memDst->exp_->Munch(instr_list, fs);
      temp::Temp *temp = temp::TempFactory::NewTemp();
      instr_list.Append(new assem::MoveInstr("movq (`s0), `d0",
                                             new temp::TempList(temp),
                                             new temp::TempList(srcTemp)));
      instr_list.Append(new assem::MoveInstr("movq `s0, (`d0)",
                                             new temp::TempList(dstTemp),
                                             new temp::TempList(temp)));
    } else {
      temp::Temp *srcTemp = src_->Munch(instr_list, fs);
      temp::Temp *dstTemp = memDst->exp_->Munch(instr_list, fs);
      instr_list.Append(new assem::MoveInstr("movq `s0, (`d0)",
                                             new temp::TempList(dstTemp),
                                             new temp::TempList(srcTemp)));
    }
  } else if (typeid(*dst_) == typeid(TempExp)) {
    temp::Temp *dst = dst_->Munch(instr_list, fs);
    temp::Temp *src = src_->Munch(instr_list, fs);
    if(src==reg_manager->StackPointer()){
      std::string movq = "movq " + std::string(fs.data()) + "(`s0), `d0";
      instr_list.Append(new assem::MoveInstr(
        movq, new temp::TempList(dst), new temp::TempList(reg_manager->StackPointer())));
    }
    else{
      instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(dst), new temp::TempList(src)));
    }
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *temp = temp::TempFactory::NewTemp();
  if (op_ == tree::BinOp::PLUS_OP) {
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);
    temp::TempList *dst = new temp::TempList(temp);
    temp::TempList *src = new temp::TempList({left, right});
    assem::MoveInstr *instr1 = new assem::MoveInstr("movq `s0, `d0", dst, src);
    assem::OperInstr *instr2 =
        new assem::OperInstr("addq `s1, `d0", dst, src, nullptr);
    instr_list.Append(instr1);
    instr_list.Append(instr2);
    return temp;
  } else if (op_ == tree::BinOp::MINUS_OP) {
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);
    temp::TempList *dst = new temp::TempList(temp);
    temp::TempList *src = new temp::TempList({left, right});
    assem::MoveInstr *instr1 = new assem::MoveInstr("movq `s0, `d0", dst, src);
    assem::OperInstr *instr2 =
        new assem::OperInstr("subq `s1, `d0", dst, src, nullptr);
    instr_list.Append(instr1);
    instr_list.Append(instr2);
    return temp;
  } else if (op_ == tree::BinOp::MUL_OP) {
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);
    temp::TempList *dst = new temp::TempList(temp);
    temp::TempList *src = new temp::TempList({left, right});
    // assem::MoveInstr *instr1 = new assem::MoveInstr("movq `s0, `d0", dst, src);
    // assem::OperInstr *instr2 =
    //     new assem::OperInstr("imulq `s1, `d0", dst, src, nullptr);
    temp::Temp *storeRdx = temp::TempFactory::NewTemp();
    assem::MoveInstr *instr1 =
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(storeRdx),
                             new temp::TempList(reg_manager->GetRDX()));
    assem::MoveInstr *instr2 = new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), src);
    assem::OperInstr *instr3 =
        new assem::OperInstr("imul `s1", nullptr, src, nullptr);
    assem::MoveInstr *instr4 = new assem::MoveInstr(
        "movq `s0, `d0", dst, new temp::TempList(reg_manager->ReturnValue()));
    assem::MoveInstr *instr5 = new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg_manager->GetRDX()),
        new temp::TempList(storeRdx));
    instr_list.Append(instr1);
    instr_list.Append(instr2);
    instr_list.Append(instr3);
    instr_list.Append(instr4);
    instr_list.Append(instr5);
    return temp;
  } else if (op_ == tree::BinOp::DIV_OP) {
    temp::Temp *left = left_->Munch(instr_list, fs);
    temp::Temp *right = right_->Munch(instr_list, fs);
    temp::TempList *dst = new temp::TempList(temp);
    temp::TempList *src = new temp::TempList({left, right});
    temp::Temp *storeRdx = temp::TempFactory::NewTemp();
    assem::MoveInstr *instr1 =
        new assem::MoveInstr("movq `s0, `d0", new temp::TempList(storeRdx),
                             new temp::TempList(reg_manager->GetRDX()));
    assem::MoveInstr *instr2 = new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), src);
    assem::OperInstr *instr3 =
        new assem::OperInstr("cqto", nullptr, nullptr, nullptr);
    assem::OperInstr *instr4 =
        new assem::OperInstr("idivq `s1", nullptr, src, nullptr);
    assem::MoveInstr *instr5 = new assem::MoveInstr(
        "movq `s0, `d0", dst, new temp::TempList(reg_manager->ReturnValue()));
    assem::MoveInstr *instr6 = new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList(reg_manager->GetRDX()),
        new temp::TempList(storeRdx));
    instr_list.Append(instr1);
    instr_list.Append(instr2);
    instr_list.Append(instr3);
    instr_list.Append(instr4);
    instr_list.Append(instr5);
    instr_list.Append(instr6);
    return temp;
  } else {
    return temp;
  }
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *temp = temp::TempFactory::NewTemp();
  if (typeid(*exp_) == typeid(BinopExp) &&
      ((BinopExp *)exp_)->op_ == BinOp::PLUS_OP &&
      typeid(*(((BinopExp *)exp_)->left_)) == typeid(ConstExp)) {
    Exp *exp1 = ((BinopExp *)exp_)->right_;
    temp::Temp *temp1 = exp1->Munch(instr_list, fs);
    int i = ((ConstExp *)(((BinopExp *)exp_)->left_))->consti_;
    if(temp1==reg_manager->FramePointer()){
      // FIXME
      std::string movq = "movq (" + std::string(fs.data()) + "+" + std::to_string(i) + ")(`s0), `d0";
      instr_list.Append(new assem::MoveInstr(movq, new temp::TempList(temp), new temp::TempList(reg_manager->StackPointer())));
    }
    else{
      std::string movq = "movq " + std::to_string(i) + "(`s0), `d0";
      instr_list.Append(new assem::MoveInstr(movq, new temp::TempList(temp),
                                           new temp::TempList(temp1)));
    }
  } else if (typeid(*exp_) == typeid(BinopExp) &&
             ((BinopExp *)exp_)->op_ == BinOp::PLUS_OP &&
             typeid(*(((BinopExp *)exp_)->right_)) == typeid(ConstExp)) {
    Exp *exp1 = ((BinopExp *)exp_)->left_;
    temp::Temp *temp1 = exp1->Munch(instr_list, fs);
    int i = ((ConstExp *)(((BinopExp *)exp_)->right_))->consti_;
    if(temp1==reg_manager->FramePointer()){
      // FIXME
      std::string movq = "movq (" + std::string(fs.data()) + "+" + std::to_string(i) + ")(`s0), `d0";
      instr_list.Append(new assem::MoveInstr(movq, new temp::TempList(temp), new temp::TempList(reg_manager->StackPointer())));
    }
    else{
      std::string movq = "movq " + std::to_string(i) + "(`s0), `d0";
      instr_list.Append(new assem::MoveInstr(movq, new temp::TempList(temp),
                                           new temp::TempList(temp1)));
    }
  } else {
    temp::Temp *temp1 = exp_->Munch(instr_list, fs);
    instr_list.Append(new assem::MoveInstr("movq (`s0), `d0",
                                           new temp::TempList(temp),
                                           new temp::TempList(temp1)));
  }
  return temp;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  return temp_;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  // rip relative addressing
  temp::Temp *temp = temp::TempFactory::NewTemp();
  std::string leaq = "leaq " + name_->Name() + "(%rip), `d0";
  instr_list.Append(
      new assem::OperInstr(leaq, new temp::TempList(temp), nullptr, nullptr));
  return temp;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *temp = temp::TempFactory::NewTemp();
  std::string str = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(
      new assem::OperInstr(str, new temp::TempList(temp), nullptr, nullptr));
  return temp;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *temp = temp::TempFactory::NewTemp();
  temp::TempList *argRegs = reg_manager->ArgRegs();
  temp::TempList *formals = args_->MunchArgs(instr_list, fs);
  int size = formals->GetList().size();
  int extra_arg;
  std::string extra_arg_str;
  if(size > 6) {
    extra_arg = size - 6;
    extra_arg_str = "subq $" + std::to_string(extra_arg * reg_manager->WordSize()) + ", %rsp";
    instr_list.Append(new assem::OperInstr(extra_arg_str, nullptr, nullptr, nullptr));
  }
  for (int i = 0; i < size; ++i) {
    if (i <= 5) {
      if(i==0 && formals->NthTemp(0)==reg_manager->FramePointer()){
        std::string leaq = "leaq " + std::string(fs.data()) + "(`s0), `d0";
        instr_list.Append(new assem::MoveInstr(leaq, argRegs, new temp::TempList(reg_manager->StackPointer())));
      }
      else{
        std::string movq =
            "movq `s" + std::to_string(i) + ", `d" + std::to_string(i);
        instr_list.Append(new assem::MoveInstr(movq, argRegs, formals));
      }
    } else {
      int offset = (i - 6) * reg_manager->WordSize();
      std::string pushq = "movq `s" + std::to_string(i) + ", " + std::to_string(offset) + "(%rsp)";
      instr_list.Append(new assem::OperInstr(pushq, nullptr, formals, nullptr));
    }
  }
  std::string call = "callq " + ((NameExp *)fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(call, nullptr, nullptr, nullptr));
  if(size > 6) {
    extra_arg = size - 6;
    extra_arg_str = "addq $" + std::to_string(extra_arg * reg_manager->WordSize()) + ", %rsp";
    instr_list.Append(new assem::OperInstr(extra_arg_str, nullptr, nullptr, nullptr));
  }
  instr_list.Append(
      new assem::MoveInstr("movq `s0, `d0", new temp::TempList(temp),
                           new temp::TempList(reg_manager->ReturnValue())));
  return temp;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *tempList = new temp::TempList();
  for (auto &exp : exp_list_) {
    temp::Temp *temp = exp->Munch(instr_list, fs);
    tempList->Append(temp);
  }
  return tempList;
}

} // namespace tree
