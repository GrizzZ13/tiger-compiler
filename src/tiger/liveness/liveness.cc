#include "tiger/liveness/liveness.h"
#include <set>

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (!Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

bool isMoveInstr(assem::Instr *instr) {
  if(typeid(*instr)==typeid(assem::MoveInstr))
    return true;
  else
    return false;
}

bool Equal(temp::TempList *a, temp::TempList *b) {
  if(a==nullptr && b==nullptr){
    return true;
  }
  else{
    std::list<temp::Temp*> aa;
    std::list<temp::Temp*> bb;
    if(a != nullptr) {
      aa = a->GetList();
    }
    if(b != nullptr) {
      bb = b->GetList();
    }
    std::set<temp::Temp*> aaa(aa.begin(), aa.end());
    std::set<temp::Temp*> bbb(bb.begin(), bb.end());
    return aaa==bbb;
  }
}

temp::TempList* Substraction(temp::TempList *a, temp::TempList *b) {
  if(a==nullptr){
    return new temp::TempList();
  }
  else if(b==nullptr){
    return a;
  }
  else{
    std::set<temp::Temp*> aa(a->GetList().begin(), a->GetList().end());
    std::list<temp::Temp*> bb = b->GetList();
    for(auto &bbb : bb){
      aa.erase(bbb);
    }
    temp::TempList *tempList = new temp::TempList();
    for(auto &aaa : aa){
      tempList->Append(aaa);
    }
    return tempList;
  }
}

temp::TempList* Union(temp::TempList *a, temp::TempList *b) {
  temp::TempList *tempList = new temp::TempList();
  std::set<temp::Temp*> returnSet;
  if(a!=nullptr){
    for(auto &temp : a->GetList()){
      returnSet.insert(temp);
    }
  }
  if(b!=nullptr){
    for(auto &temp : b->GetList()){
      returnSet.insert(temp);
    }
  }
  for(auto &temp : returnSet) {
    tempList->Append(temp);
  }
  return tempList;
};


void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  bool done = false;
  while (!done) {
    done = true;
    std::list<fg::FNodePtr> nodePointers = flowgraph_->Nodes()->GetList();
    for(auto &nodePtr : nodePointers) {
      // first rule
      assem::Instr *instr = nodePtr->NodeInfo();
      temp::TempList *in = Union(instr->Use(), Substraction(out_->Look(nodePtr), instr->Def()));
      if(!Equal(in, in_->Look(nodePtr))){
        done = false;
        in_->Enter(nodePtr, in);
      }

      // second rule
      fg::FNodeListPtr succListPtr = nodePtr->Succ();
      std::list<fg::FNodePtr> succList = succListPtr->GetList();
      temp::TempList *out = nullptr;
      for(auto &succ : succList) {
        out = Union(out, in_->Look(succ));
      }
      if(!Equal(out, out_->Look(nodePtr))) {
        done = false;
        out_->Enter(nodePtr, out);
        my_out[nodePtr->NodeInfo()] = out;
      }
    }
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */

  // precolored conflict
  for(auto &temp : reg_manager->Registers()->GetList()){
    INodePtr inode = live_graph_.interf_graph->NewNode(temp);
    temp_node_map_->Enter(temp, inode); 
  }

  for(auto &temp1 : reg_manager->Registers()->GetList()){
    for(auto &temp2 : reg_manager->Registers()->GetList()){
      INodePtr inode1 = temp_node_map_->Look(temp1);
      INodePtr inode2 = temp_node_map_->Look(temp2);
      if(inode1 != inode2){
        live_graph_.interf_graph->AddEdge(inode1, inode2);
        live_graph_.interf_graph->AddEdge(inode2, inode1);
      }
    }
  }


  std::list<fg::FNodePtr> fnodes = flowgraph_->Nodes()->GetList();
  std::set<temp::Temp*> tempSet;
  for(auto &fnode : fnodes) {
    temp::TempList *tempList = fnode->NodeInfo()->Use();
    if(tempList != nullptr) {
      std::list<temp::Temp*> list = tempList->GetList();
      tempSet.insert(list.begin(), list.end());
    }
    tempList = fnode->NodeInfo()->Def();
    if(tempList != nullptr) {
      std::list<temp::Temp*> list = tempList->GetList();
      tempSet.insert(list.begin(), list.end());
    }
  }
  for(auto &temp : tempSet) {
    if(reg_manager->IsMachineRegister(temp)){
      continue;
    }
    INodePtr inode = live_graph_.interf_graph->NewNode(temp);
    temp_node_map_->Enter(temp, inode);
  }
  for(auto &fnode : fnodes) {
    temp::TempList *defs = fnode->NodeInfo()->Def();
    temp::TempList *alives = out_->Look(fnode);
    if(defs!=nullptr){
      bool move = isMoveInstr(fnode->NodeInfo());
      temp::Temp *use;
      if(move){
        use = fnode->NodeInfo()->Use()->NthTemp(0);
        temp::Temp *def = fnode->NodeInfo()->Def()->NthTemp(0);
        INodePtr defNode = temp_node_map_->Look(def);
        INodePtr useNode = temp_node_map_->Look(use);
        live_graph_.moves->Append(useNode, defNode);
        if(moveList[defNode]==nullptr){
          moveList[defNode] = new MoveList();
        }
        if(moveList[useNode]==nullptr){
          moveList[useNode] = new MoveList();
        }
        moveList[defNode]->Append(useNode, defNode);
        moveList[useNode]->Append(useNode, defNode);
      }

      std::list<temp::Temp*> defList = defs->GetList();
      std::list<temp::Temp*> aliveList;
      if(alives!=nullptr){
        aliveList = alives->GetList();
      }
      for(auto &def : defList) {
        for(auto &alive : aliveList) {
          if(!move && alive != def){
            INodePtr inode1 = temp_node_map_->Look(def);
            INodePtr inode2 = temp_node_map_->Look(alive);
            live_graph_.interf_graph->AddEdge(inode1, inode2);
            live_graph_.interf_graph->AddEdge(inode2, inode1);
          }
          else if(move && alive != use && alive != def){
            INodePtr inode1 = temp_node_map_->Look(def);
            INodePtr inode2 = temp_node_map_->Look(alive);
            live_graph_.interf_graph->AddEdge(inode1, inode2);
            live_graph_.interf_graph->AddEdge(inode2, inode1);
          }
        }
      }
    }
  }
  // weight to select spill node
  std::list<fg::FNodePtr> fnpl = flowgraph_->Nodes()->GetList();
  for(auto &fnp : fnpl){
    if(fnp->NodeInfo()->Def()!=nullptr){
      std::list<temp::Temp*> tempList = fnp->NodeInfo()->Def()->GetList();
      for(auto &temp : tempList){
        INodePtr inode = temp_node_map_->Look(temp);
        weight[inode] = weight[inode] + 1.0;
      }
    }
    if(fnp->NodeInfo()->Use()!=nullptr){
      std::list<temp::Temp*> tempList = fnp->NodeInfo()->Use()->GetList();
      for(auto &temp : tempList){
        INodePtr inode = temp_node_map_->Look(temp);
        weight[inode] = weight[inode] + 1.0;
      }
    }
  }
  std::list<INodePtr> inpl = live_graph_.interf_graph->Nodes()->GetList();
  for(auto &inp : inpl) {
    if(reg_manager->IsMachineRegister(inp->NodeInfo())){
      continue;
    }
    INodePtr inode = temp_node_map_->Look(inp->NodeInfo());
    weight[inode] = weight[inode] / inp->InDegree();
  }
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live