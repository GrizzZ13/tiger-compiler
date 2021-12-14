#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"
#include <set>
#include <map>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result(){};
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
public:
  explicit RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr) {
    this->frame = (frame::X64Frame*)frame;
    this->assemInstr = std::move(assem_instr);
    this->instrList = assemInstr->GetInstrList();
  }
  void RegAlloc();
  std::unique_ptr<ra::Result> TransferResult(){
    return std::move(result);
  }
private:
  void MakeWorklist();
  void Simplify();
  void Coalesce();
  void Freeze();
  void SelectSpill();
  void AssignColors();
  void DecrementDegree(live::INodePtr inode);
  void FreezeMoves(live::INodePtr inode);
  void EnableMoves(std::set<live::INodePtr> inodes);
  void AddWorklist(live::INodePtr inode);
  void Combine(live::INodePtr u, live::INodePtr v);
  void RewriteProgram();

  int GetDegree(live::INodePtr inode);
  bool OK(live::INodePtr t, live::INodePtr u);
  bool Conservative(std::set<live::INodePtr> set);
  bool AllOK(std::set<live::INodePtr> adjs, live::INodePtr u);
  bool MoveRelated(live::INodePtr inode);
  bool Precolored(live::INodePtr inode);
  bool Contain(temp::Temp *temp, temp::TempList *tempList);
  live::MoveList* NodeMoves(live::INodePtr n);
  temp::TempList* Replace(temp::Temp *temp, temp::TempList *tempList);
  live::INodePtr GetAlias(live::INodePtr inode);
  std::set<live::INodePtr> AdjUnion(live::INodePtr u, live::INodePtr v);
  std::set<live::INodePtr> Adjacent(live::INodePtr inode);

  live::LiveGraph liveGraph;
  std::unique_ptr<cg::AssemInstr> assemInstr;
  frame::X64Frame *frame;
  std::unique_ptr<ra::Result> result;
  temp::Map *coloring;
  assem::InstrList *instrList;

  std::map<live::INodePtr, double> weight;
  std::map<live::INodePtr, temp::Temp*> node2reg;
  std::map<live::INodePtr, int> degree;
  std::map<live::INodePtr, live::INodePtr> alias;
  std::set<live::INodePtr> spillWorklist, freezeWorklist, simplifyWorklist;
  std::set<live::INodePtr> spilledNodes, coalescedNodes, coloredNodes;
  std::map<live::INodePtr, live::MoveList*> moveList;
  std::vector<live::INodePtr> selectedStack;
  live::MoveList coalescedMoves, constrainedMoves, frozenMoves, worklistMoves, activeMoves;
};

} // namespace ra

#endif