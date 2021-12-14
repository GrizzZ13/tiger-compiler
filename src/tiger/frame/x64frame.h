//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : Access(true), offset(offset) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override;
};

class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : Access(false), reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override;
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame();
  ~X64Frame();
  Access* allocLocal(bool escape) override;
  static tree::Exp* externalCall(std::string labelName, tree::ExpList *args);
  static Frame* NewFrame(temp::Label *name, std::list<bool>& escapes);
  temp::Temp* FramePointer() const override;
  tree::Stm* procEntryExit1(tree::Stm *stm) override;
  assem::Proc* ProcEntryExit3(assem::InstrList *il) override;
  std::string GetLabel() override;
};

class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  temp::Temp* rsp;
  temp::Temp* rbp;
  temp::Temp* rax;
  temp::Temp* rdi;
  temp::Temp* rsi;
  temp::Temp* rdx;
  temp::Temp* rcx;
  temp::Temp* r8;
  temp::Temp* r9;
  temp::Temp* r10;
  temp::Temp* r11;
  temp::Temp* r12;
  temp::Temp* r13;
  temp::Temp* r14;
  temp::Temp* r15;
  temp::Temp* rbx;

  std::string rsp_str;
  std::string rbp_str;
  std::string rax_str;
  std::string rdi_str;
  std::string rsi_str;
  std::string rdx_str;
  std::string rcx_str;
  std::string r8_str;
  std::string r9_str;
  std::string r10_str;
  std::string r11_str;
  std::string r12_str;
  std::string r13_str;
  std::string r14_str;
  std::string r15_str;
  std::string rbx_str;

  temp::TempList* regs;
  temp::TempList* argRegs;
  temp::TempList* callerSaves;
  temp::TempList* calleeSaves;
  temp::TempList* returnSink;

public:
  static const int K = 15;
  X64RegManager() {
    rsp_str = "%rsp";
    rbp_str = "%rbp";
    rax_str = "%rax";
    rdi_str = "%rdi";
    rsi_str = "%rsi";
    rdx_str = "%rdx";
    rcx_str = "%rcx";
    r8_str = "%r8";
    r9_str = "%r9";
    r10_str = "%r10";
    r11_str = "%r11";
    r12_str = "%r12";
    r13_str = "%r13";
    r14_str = "%r14";
    r15_str = "%r15";
    rbx_str = "%rbx";
    rsp = temp::TempFactory::NewTemp();
    rbp = temp::TempFactory::NewTemp();
    rax = temp::TempFactory::NewTemp();
    rdi = temp::TempFactory::NewTemp();
    rsi = temp::TempFactory::NewTemp();
    rdx = temp::TempFactory::NewTemp();
    rcx = temp::TempFactory::NewTemp();
    r8 = temp::TempFactory::NewTemp();
    r9 = temp::TempFactory::NewTemp();
    r10 = temp::TempFactory::NewTemp();
    r11 = temp::TempFactory::NewTemp();
    r12 = temp::TempFactory::NewTemp();
    r13 = temp::TempFactory::NewTemp();
    r14 = temp::TempFactory::NewTemp();
    r15 = temp::TempFactory::NewTemp();
    rbx = temp::TempFactory::NewTemp();
    temp::Map::Name()->Enter(rsp, &rsp_str);
    temp::Map::Name()->Enter(rbp, &rbp_str);
    temp::Map::Name()->Enter(rax, &rax_str);
    temp::Map::Name()->Enter(rdi, &rdi_str);
    temp::Map::Name()->Enter(rsi, &rsi_str);
    temp::Map::Name()->Enter(rdx, &rdx_str);
    temp::Map::Name()->Enter(rcx, &rcx_str);
    temp::Map::Name()->Enter(r8,  &r8_str);
    temp::Map::Name()->Enter(r9,  &r9_str);
    temp::Map::Name()->Enter(r10, &r10_str);
    temp::Map::Name()->Enter(r11, &r11_str);
    temp::Map::Name()->Enter(r12, &r12_str);
    temp::Map::Name()->Enter(r13, &r13_str);
    temp::Map::Name()->Enter(r14, &r14_str);
    temp::Map::Name()->Enter(r15, &r15_str);
    temp::Map::Name()->Enter(rbx, &rbx_str);
    
    temp_map_->Enter(rsp, &rsp_str);
    temp_map_->Enter(rbp, &rbp_str);
    temp_map_->Enter(rax, &rax_str);
    temp_map_->Enter(rdi, &rdi_str);
    temp_map_->Enter(rsi, &rsi_str);
    temp_map_->Enter(rdx, &rdx_str);
    temp_map_->Enter(rcx, &rcx_str);
    temp_map_->Enter(r8,  &r8_str);
    temp_map_->Enter(r9,  &r9_str);
    temp_map_->Enter(r10, &r10_str);
    temp_map_->Enter(r11, &r11_str);
    temp_map_->Enter(r12, &r12_str);
    temp_map_->Enter(r13, &r13_str);
    temp_map_->Enter(r14, &r14_str);
    temp_map_->Enter(r15, &r15_str);
    temp_map_->Enter(rbx, &rbx_str);
  }

  bool IsMachineRegister(temp::Temp *temp);

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] temp::TempList *Registers();

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] temp::TempList *ArgRegs();

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] temp::TempList *CallerSaves();

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] temp::TempList *CalleeSaves();

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] temp::TempList *ReturnSink();

  /**
   * Get word size
   */
  [[nodiscard]] int WordSize();

  [[nodiscard]] temp::Temp *FramePointer();

  [[nodiscard]] temp::Temp *StackPointer();

  [[nodiscard]] temp::Temp *ReturnValue();

  [[nodiscard]] temp::Temp *GetRDX();
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
