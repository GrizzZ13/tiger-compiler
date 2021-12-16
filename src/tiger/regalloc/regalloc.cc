#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

// #define DEBUG

#ifdef DEBUG
std::vector<temp::Temp*> spilledTemp;
std::set<temp::Temp*> newIntroducedTemp;
assem::InstrList *debugList;
std::map<assem::Instr*, temp::TempList*> liveout;

void showSpilledTemp(FILE *out) {
    fprintf(out, "================spilled temps================\n");
    for(auto temp : spilledTemp) {
        if(temp){
            std::string name = *(temp::Map::Name()->Look(temp));
            fprintf(out, "%s ", name.c_str());
        }
    }
    fprintf(out, "\n==============================================\n");
}

void showNewTemp(FILE *out) {
    fprintf(out, "===================new temps==================\n");
    for(auto temp : newIntroducedTemp) {
        if(temp){
            std::string name = *(temp::Map::Name()->Look(temp));
            fprintf(out, "%s ", name.c_str());
        }
    }
    fprintf(out, "\n==============================================\n");
}

void showTempList(temp::TempList *tempList, FILE *out, std::set<temp::Temp*> target){
    fprintf(out, "temps : \n");
    bool flag = false;
    if(tempList){
        std::list<temp::Temp*> stdlist = tempList->GetList();
        for(auto temp : stdlist) {
            if(temp){
                if(!flag && target.find(temp)!=target.end()){
                    flag = true;
                }
                std::string name = *(temp::Map::Name()->Look(temp));
                fprintf(out, "%s ", name.c_str());
            }
        }
    }
    if(flag) {
        fprintf(out, "        <<<<<<<<\n\n");
    }
    else{
        fprintf(out, "\n\n");
    }
}

void showInstrAndTemp(FILE *out){
    if(debugList){
        std::list<assem::Instr*> stdlist = debugList->GetList();
        for(auto ins : stdlist) {
            ins->Print(out, temp::Map::Name());
            temp::TempList *tl = liveout[ins];
            fprintf(out, "live out ");
            showTempList(tl, out, newIntroducedTemp);
        }
    }
}

void SHOWALL(){
    showSpilledTemp(stdout);
    showNewTemp(stdout);
    showInstrAndTemp(stdout);
}

#endif

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
void RegAllocator::RegAlloc() {
    coalescedMoves = new live::MoveList();
    constrainedMoves = new live::MoveList();
    frozenMoves = new live::MoveList();
    worklistMoves = new live::MoveList();
    activeMoves = new live::MoveList();

    LivenessAnalysis();
    Build();
    MakeWorklist();

    while(true){
        if(!simplifyWorklist.empty()) {
            Simplify();
        }
        else if(!worklistMoves->GetList().empty()){
            Coalesce();
        }
        else if(!freezeWorklist.empty()) {
            Freeze();
        }
        else if(!spillWorklist.empty()) {
            SelectSpill();
        }

        if(simplifyWorklist.empty() && worklistMoves->GetList().empty() 
        && freezeWorklist.empty() && spillWorklist.empty()){
            break;
        }
    }
    AssignColors();
    if(!spilledNodes.empty()){
        RewriteProgram();
        RegAlloc();
    }
    else{
        result = std::make_unique<ra::Result>(coloring, instrList);
    }
}

void RegAllocator::LivenessAnalysis() {
    fg::FlowGraphFactory fgf(instrList);
    fgf.AssemFlowGraph();
    fg::FGraphPtr fgPtr = fgf.GetFlowGraph();
    lgf = new live::LiveGraphFactory(fgPtr);
    lgf->Liveness();
    #ifdef DEBUG
    debugList = instrList;
    liveout = lgf->my_out;
    SHOWALL();
    #endif
}

void RegAllocator::Build() {
    coloring = temp::Map::Empty();
    liveGraph = lgf->GetLiveGraph();
    worklistMoves = liveGraph.moves;
    moveList = lgf->GetMoveListMap();
    weight = lgf->GetWeight();
}

bool RegAllocator::Precolored(live::INodePtr inode) {
    return reg_manager->IsMachineRegister(inode->NodeInfo());
}

void RegAllocator::Simplify() {
    live::INodePtr inode = *(simplifyWorklist.begin());
    simplifyWorklist.erase(inode);
    selectedStack.push_back(inode);
    std::set<live::INodePtr> set = Adjacent(inode);
    for(auto &m : set) {
        DecrementDegree(m);
    }
}

void RegAllocator::MakeWorklist() {
    std::list<live::INodePtr> inodes = liveGraph.interf_graph->Nodes()->GetList();
    for(auto &inode :inodes) {
        if(Precolored(inode)){
            node2reg[inode] = inode->NodeInfo();
            continue;
        }
        if(inode->InDegree() >= frame::X64RegManager::K){
            spillWorklist.insert(inode);
        }
        else if(MoveRelated(inode)) {
            freezeWorklist.insert(inode);
        }
        else {
            simplifyWorklist.insert(inode);
        }
    }
}

live::MoveList* RegAllocator::NodeMoves(live::INodePtr n) {
    if(moveList[n]==nullptr){
        moveList[n] = new live::MoveList();
    }
    return moveList[n]->Intersect(activeMoves->Union(worklistMoves));
}

bool RegAllocator::MoveRelated(live::INodePtr inode){
    return NodeMoves(inode)->GetList().size()!=0;
}

std::set<live::INodePtr> RegAllocator::Adjacent(live::INodePtr inode){
    std::list<live::INodePtr> preds = inode->Pred()->GetList();
    std::set<live::INodePtr> set(preds.begin(), preds.end());
    for(auto &n : selectedStack) {
        set.erase(n);
    }
    for(auto &n : coalescedNodes) {
        set.erase(n);
    }
    return set;
}

int RegAllocator::GetDegree(live::INodePtr inode) {
    std::list<live::INodePtr> preds = inode->Pred()->GetList();
    std::set<live::INodePtr> set(preds.begin(), preds.end());
    for(auto &n : selectedStack) {
        set.erase(n);
    }
    for(auto &n : coalescedNodes) {
        set.erase(n);
    }
    return set.size();
}

void RegAllocator::DecrementDegree(live::INodePtr inode) {
    int d = GetDegree(inode);
    if(d == frame::X64RegManager::K-1){
        std::set<live::INodePtr> set = Adjacent(inode);
        set.insert(inode);
        EnableMoves(set);
        spillWorklist.erase(inode);
        if(MoveRelated(inode)){
            freezeWorklist.insert(inode);
        }
        else{
            simplifyWorklist.insert(inode);
        }
    }
}

void RegAllocator::EnableMoves(std::set<live::INodePtr> inodes) {
    for(auto &inode : inodes) {
        live::MoveList *nms = NodeMoves(inode);
        for(auto &m : nms->GetList()){
            if(activeMoves->Contain(m.first, m.second)){
                activeMoves->Delete(m.first, m.second);
                if(!worklistMoves->Contain(m.first, m.second)){
                    worklistMoves->Append(m.first, m.second);
                }
            }
        }
    }
}

void RegAllocator::Coalesce() {
    auto pair = *(worklistMoves->GetList().begin());
    live::INodePtr x = GetAlias(pair.first);
    live::INodePtr y = GetAlias(pair.second);
    live::INodePtr u, v;
    if(Precolored(y)){
        u = y;
        v = x;
    }
    else{
        u = x;
        v = y;
    }
    worklistMoves->Delete(pair.first, pair.second);
    if(u==v){
        if(!coalescedMoves->Contain(pair.first, pair.second)){
            coalescedMoves->Append(pair.first, pair.second);
        }
        AddWorklist(u);
    }
    else if(Precolored(v) || u->GoesTo(v)){
        if(!constrainedMoves->Contain(pair.first, pair.second)){
            constrainedMoves->Append(pair.first, pair.second);
        }
        AddWorklist(u);
        AddWorklist(v);
    }
    else if( (Precolored(u) && AllOK(Adjacent(v), u)) || 
    (!Precolored(u) && Conservative(AdjUnion(u, v)))) {
        if(!coalescedMoves->Contain(pair.first, pair.second)){
            coalescedMoves->Append(pair.first, pair.second);
        }
        Combine(u, v);
        AddWorklist(u);
    }
    else{
        if(!activeMoves->Contain(pair.first, pair.second)){
            activeMoves->Append(pair.first, pair.second);
        }
    }
}

std::set<live::INodePtr> RegAllocator::AdjUnion(live::INodePtr u, live::INodePtr v) {
    std::set<live::INodePtr> set, tmp1, tmp2;
    tmp1 = Adjacent(u);
    tmp2 = Adjacent(v);
    set.insert(tmp1.begin(), tmp1.end());
    set.insert(tmp2.begin(), tmp2.end());
    return set;
}

bool RegAllocator::AllOK(std::set<live::INodePtr> adjs, live::INodePtr u) {
    bool ok = true;
    for(auto &adj : adjs) {
        if(!OK(adj, u)){
            ok = false;
            break;
        }
    }
    return ok;
}

bool RegAllocator::OK(live::INodePtr t, live::INodePtr u) {
    return GetDegree(t) < frame::X64RegManager::K || Precolored(t) || t->GoesTo(u);
}

bool RegAllocator::Conservative(std::set<live::INodePtr> set) {
    int k = 0;
    for(auto &node : set) {
        if(GetDegree(node)>= frame::X64RegManager::K){
            k++;
        }
    }
    return k < frame::X64RegManager::K;
}

void RegAllocator::AddWorklist(live::INodePtr u) {
    if(!Precolored(u) && !MoveRelated(u) && GetDegree(u) < frame::X64RegManager::K){
        freezeWorklist.erase(u);
        simplifyWorklist.insert(u);
    }
}

void RegAllocator::Combine(live::INodePtr u, live::INodePtr v) {
    if(freezeWorklist.find(v) != freezeWorklist.end()) {
        freezeWorklist.erase(v);
    }
    else{
        spillWorklist.erase(v);
    }
    coalescedNodes.insert(v);
    alias[v] = u;
    moveList[u]->Union(moveList[v]);
    std::set<live::INodePtr> tmpSet;
    tmpSet.insert(v);
    EnableMoves(tmpSet);
    std::set<live::INodePtr> adjs = Adjacent(v);
    for(auto &adj : adjs){
        liveGraph.interf_graph->AddEdge(adj, u);
        liveGraph.interf_graph->AddEdge(u, adj);
        DecrementDegree(adj);
    }
    if(GetDegree(u) >= frame::X64RegManager::K && freezeWorklist.find(u)!=freezeWorklist.end()){
        freezeWorklist.erase(u);
        spillWorklist.insert(u);
    }
}

live::INodePtr RegAllocator::GetAlias(live::INodePtr inode) {
    if(coalescedNodes.find(inode)!=coalescedNodes.end()) {
        return GetAlias(alias[inode]);
    }
    else{
        return inode;
    }
}

void RegAllocator::Freeze() {
    live::INodePtr u = *(freezeWorklist.begin());
    freezeWorklist.erase(u);
    simplifyWorklist.insert(u);
    FreezeMoves(u);
}

void RegAllocator::FreezeMoves(live::INodePtr u) {
    live::MoveList *ml = NodeMoves(u);
    std::list<std::pair<live::INodePtr, live::INodePtr>> mll = ml->GetList();
    for(auto pair : mll) {
        live::INodePtr x = pair.first;
        live::INodePtr y = pair.second;
        live::INodePtr v;
        if(GetAlias(y)==GetAlias(u)){
            v = GetAlias(x);
        }
        else{
            v = GetAlias(y);
        }
        if(activeMoves->Contain(x, y)){
            activeMoves->Delete(x, y);
        }
        if(!frozenMoves->Contain(x, y)){
            frozenMoves->Append(x, y);
        }
        if(NodeMoves(v)->GetList().empty() && GetDegree(v) < frame::X64RegManager::K){
            freezeWorklist.erase(v);
            simplifyWorklist.insert(v);
        }
    }
}

void RegAllocator::SelectSpill() {
    double benchmark = 0;
    live::INodePtr selected = nullptr;
    for(auto &p : spillWorklist){
        if(notSpill.find(p->NodeInfo())!=notSpill.end()){
            continue;
        }
        if(weight[p] >= benchmark){
            selected = p;
            benchmark = weight[p];
        }
    }
    if(!selected){
        selected = *(spillWorklist.begin());
    }
    spillWorklist.erase(selected);
    simplifyWorklist.insert(selected);
    FreezeMoves(selected);
}

void RegAllocator::AssignColors() {    
    while(!selectedStack.empty()){
        live::INodePtr n = selectedStack.back();
        selectedStack.pop_back();
        std::list<temp::Temp*> okColor = reg_manager->Registers()->GetList();
        for(auto &w : n->Pred()->GetList()){
            if(Precolored(GetAlias(w)) || coloredNodes.find(GetAlias(w))!=coloredNodes.end()){
                okColor.remove(node2reg[GetAlias(w)]);
            }
        }
        if(okColor.empty()){
            spilledNodes.insert(n);
        }
        else{
            coloredNodes.insert(n);
            temp::Temp *temp = okColor.back();
            node2reg[n] = temp;
        }
    }
    for(auto &node : coalescedNodes){
        node2reg[node] = node2reg[GetAlias(node)];
    }
    for(auto pair : node2reg){
        std::string *str = reg_manager->temp_map_->Look(pair.second);
        coloring->Enter(pair.first->NodeInfo(), str);
    }
}

bool RegAllocator::Contain(temp::Temp *temp, temp::TempList *tempList) {
    if(tempList==nullptr){
        return false;
    }
    for(auto &t : tempList->GetList()){
        if(t==temp){
            return true;
        }
    }
    return false;
}

void RegAllocator::RewriteProgram() {
    for(auto &node : spilledNodes) {
        temp::Temp *spilled = node->NodeInfo();
        frame->offset_ -= reg_manager->WordSize();
        std::list<assem::Instr*> il = instrList->GetList();
        for(auto itr=instrList->GetList().begin();itr!=instrList->GetList().end();++itr){
            assem::Instr *tmp = (*itr);
            temp::TempList *src = tmp->Use();
            temp::TempList *dst = tmp->Def();
            if(src && Contain(spilled, src) && dst && Contain(spilled, dst)){
                temp::Temp *newTemp = temp::TempFactory::NewTemp();
                #ifdef DEBUG
                newIntroducedTemp.insert(newTemp);
                #endif
                notSpill.insert(newTemp);
                tmp->Replace(spilled, newTemp);
                std::string assem1 = "movq ("+frame->name_->Name()+"_framesize+"+std::to_string(frame->offset_)+")(`s0), `d0";
                std::string assem2 = "movq `s0, ("+frame->name_->Name()+"_framesize+"+std::to_string(frame->offset_)+")(`s1)";
                assem::Instr *instr1 = new assem::OperInstr(assem1, new temp::TempList(newTemp), new temp::TempList(reg_manager->StackPointer()), nullptr);
                assem::Instr *instr2 = new assem::OperInstr(assem2, nullptr, new temp::TempList({newTemp, reg_manager->StackPointer()}), nullptr);
                instrList->Insert(itr, instr1);
                itr++;
                instrList->Insert(itr, instr2);
                itr--;
            }
            else if(src && Contain(spilled, src)){
                temp::Temp *newTemp = temp::TempFactory::NewTemp();
                #ifdef DEBUG
                newIntroducedTemp.insert(newTemp);
                #endif
                notSpill.insert(newTemp);
                tmp->Replace(spilled, newTemp);
                std::string assem1 = "movq ("+frame->name_->Name()+"_framesize+"+std::to_string(frame->offset_)+")(`s0), `d0";
                assem::Instr *instr1 = new assem::OperInstr(assem1, new temp::TempList(newTemp), new temp::TempList(reg_manager->StackPointer()), nullptr);
                instrList->Insert(itr, instr1);
            }
            else if(dst && Contain(spilled, dst)){
                temp::Temp *newTemp = temp::TempFactory::NewTemp();
                #ifdef DEBUG
                newIntroducedTemp.insert(newTemp);
                #endif
                notSpill.insert(newTemp);
                tmp->Replace(spilled, newTemp);
                std::string assem2 = "movq `s0, ("+frame->name_->Name()+"_framesize+"+std::to_string(frame->offset_)+")(`s1)";
                assem::Instr *instr2 = new assem::OperInstr(assem2, nullptr, new temp::TempList({newTemp, reg_manager->StackPointer()}), nullptr);
                itr++;
                instrList->Insert(itr, instr2);
                itr--;
            }
        }
    }
    #ifdef DEBUG
    spilledTemp.clear();
    for(auto t : spilledNodes){
        spilledTemp.push_back(t->NodeInfo());
    }
    #endif
    spilledNodes.clear();
    coloredNodes.clear();
    coalescedNodes.clear();
    weight.clear();
    node2reg.clear();
    alias.clear();
    moveList.clear();
}

} // namespace ra