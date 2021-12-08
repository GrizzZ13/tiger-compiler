#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  std::list<assem::Instr*> instrList = instr_list_->GetList();
  for(auto &instr : instrList) {
    fg::FNodePtr node = flowgraph_->NewNode(instr);
    if(typeid(*instr)==typeid(assem::LabelInstr)) {
      label_map_->Enter(((assem::LabelInstr*)instr)->label_, node);
    }
  }
  std::list<fg::FNodePtr> nodePtrList = flowgraph_->Nodes()->GetList();
  int index = 0;
  for(auto &nodePtr : nodePtrList) {
    assem::Instr *instr = nodePtr->NodeInfo();
    if(typeid(*instr)==typeid(assem::OperInstr)
      && ((assem::OperInstr*)instr)->jumps_!= nullptr) {
        std::vector<temp::Label*> labels = ((assem::OperInstr*)instr)->jumps_->labels_;
        for(auto &label : labels) {
          fg::FNodePtr jmpNode = label_map_->Look(label);
          flowgraph_->AddEdge(nodePtr, jmpNode);
        }
    }
    else{
      auto itr = nodePtrList.begin();
      std::advance(itr, index+1);
      flowgraph_->AddEdge(nodePtr, *itr);
    }
    index++;
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return src_;
}
} // namespace assem
