#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */

Frame::Frame() {
  viewShift_ = new tree::StmList();
}

Frame::~Frame() {
  delete viewShift_;
}

tree::Exp* X64Frame::externalCall(std::string labelName, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(labelName)), args);
}

X64Frame::X64Frame(){}

X64Frame::~X64Frame(){}

Frame* X64Frame::NewFrame(temp::Label *name, std::list<bool>& escapes) {
  Frame *frame = new X64Frame();
  frame->name_ = name;
  frame->offset_ = 0;
  for(auto& escape : escapes) {
    frame::Access *access;
    if(escape){
      frame->offset_ -= reg_manager->WordSize();
      access = new InFrameAccess(frame->offset_);
    }
    else{
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    frame->formals_.push_back(access);
  }

  std::list<temp::Temp*> argRegs = reg_manager->ArgRegs()->GetList();
  int i = 0;
  // view shift : transfer argments to register or memory
  for(auto &access : frame->formals_){
    tree::Stm *stm;
    tree::Exp *dstExp;
    if(access->inframe_){
      dstExp = new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(frame->FramePointer()), new tree::ConstExp(((InFrameAccess*)access)->offset)));
    }
    else{
      dstExp = new tree::TempExp(((InRegAccess*)access)->reg);
    }
    temp::Temp *argReg;
    if(i<=5){
      auto itr = argRegs.begin();
      std::advance(itr, i);
      stm = new tree::MoveStm(dstExp, new tree::TempExp(*(itr)));
    }
    else{
      stm = new tree::MoveStm(dstExp, new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(frame->FramePointer()), new tree::ConstExp((i -5) * reg_manager->WordSize()))));
    }
    i++;
    frame->viewShift_->Linear(stm);
  }
  return frame;
}

tree::Stm* X64Frame::procEntryExit1(tree::Stm *body) {
  tree::Stm *result = body;
  std::list<tree::Stm*> stmList = viewShift_->GetList();
  for(const auto &stm : stmList) {
    result = new tree::SeqStm(stm, result);
  }
  return result;
}

assem::Proc* X64Frame::ProcEntryExit3(assem::InstrList *instrList) {
  std::string prologue = name_->Name() + ":\n";
  prologue += ".set " + name_->Name() + "_framesize, " + std::to_string(-offset_) + "\n";
  prologue += "subq $" + std::to_string(-offset_) + ", %rsp\n";
  std::string epilogue = "addq $" + std::to_string(-offset_) + ", %rsp\nretq\n\n";
  return new assem::Proc(prologue, instrList, epilogue);
}

std::string X64Frame::GetLabel() {
  return name_->Name();
}

tree::Exp *InRegAccess::ToExp(tree::Exp *framePtr) const {
  return new tree::TempExp(reg);
}

tree::Exp *InFrameAccess::ToExp(tree::Exp *framePtr) const {
  return new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, framePtr, new tree::ConstExp(offset)));
}

Access* X64Frame::allocLocal(bool escape) {
  Access *access;
  if(escape){
    offset_ -= reg_manager->WordSize();
    access = new InFrameAccess(offset_);
  }
  else{
    access = new InRegAccess(temp::TempFactory::NewTemp());
  }
  locals_.push_back(access);
  return access;
}

temp::Temp* X64Frame::FramePointer() const {
  return reg_manager->FramePointer();
}

temp::TempList *X64RegManager::Registers() {
  if(regs==nullptr){
    regs = new temp::TempList({rbp, rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11, r12, r13, r14, r15, rbx});
  }
  return regs;
}

temp::TempList *X64RegManager::ArgRegs() {
  if(argRegs==nullptr){
    argRegs = new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
  }
  return argRegs;
}

temp::TempList *X64RegManager::CallerSaves() {
  if(callerSaves==nullptr){
    callerSaves = new temp::TempList({r10, r11});
  }
  return callerSaves;
}

temp::TempList *X64RegManager::CalleeSaves() {
  if(calleeSaves==nullptr){
    calleeSaves = new temp::TempList({rbx, rbp, r12, r13, r14, r15});
  }
  return calleeSaves;
}

temp::TempList *X64RegManager::ReturnSink() {
  return nullptr;
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return rbp;
}

temp::Temp *X64RegManager::StackPointer() {
  return rsp;
}

temp::Temp *X64RegManager::ReturnValue() {
  return rax;
}

temp::Temp *X64RegManager::GetRDX() {
  return rdx;
}

} // namespace frame