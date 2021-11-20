//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
public:
  X64RegManager(){}

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
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
