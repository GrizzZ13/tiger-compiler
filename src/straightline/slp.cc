#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(stm1->MaxArgs(), stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  t = stm1->Interp(t);
  return stm2->Interp(t);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *intAndTable = exp->Interp(t);
  t = intAndTable->t;
  int value = intAndTable->i;
  Table *ret = new Table(id, value, t);
  return ret;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(exps->ExpNumber(), exps->MaxArgs());
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  std::list<int> list;
  IntAndTable *intAndTable = exps->Interp(t, list);
  for(auto itr = list.begin();itr!=list.end();++itr){
    std::cout << *itr << ' ';
  }
  std::cout << std::endl;
  return intAndTable->t;
}

int A::IdExp::MaxArgs() const { return 0; }
IntAndTable *A::IdExp::Interp(Table *table) const {
  int value = table->Lookup(id);
  IntAndTable *ret = new IntAndTable(value, table);
  return ret;
}

int A::NumExp::MaxArgs() const { return 0; }
IntAndTable *A::NumExp::Interp(Table *table) const {
  IntAndTable *ret = new IntAndTable(num, table);
  return ret;
}

int A::OpExp::MaxArgs() const {
  return std::max(left->MaxArgs(), right->MaxArgs());
}
IntAndTable *A::OpExp::Interp(Table *table) const {
  IntAndTable *intAndTable = left->Interp(table);
  table = intAndTable->t;
  int valueLeft = intAndTable->i;
  delete intAndTable;

  intAndTable = right->Interp(table);
  table = intAndTable->t;
  int valueRight = intAndTable->i;
  delete intAndTable;

  int valueRet;
  switch (oper) {
  case PLUS:
    valueRet = valueLeft + valueRight;
    break;
  case MINUS:
    valueRet = valueLeft - valueRight;
    break;
  case TIMES:
    valueRet = valueLeft * valueRight;
    break;
  case DIV:
    valueRet = valueLeft / valueRight;
    break;
  }
  intAndTable = new IntAndTable(valueRet, table);
  return intAndTable;
}

int A::EseqExp::MaxArgs() const {
  return std::max(stm->MaxArgs(), exp->MaxArgs());
}

IntAndTable *A::EseqExp::Interp(Table *table) const {
  return exp->Interp(stm->Interp(table));
}

int A::PairExpList::MaxArgs() const {
  return std::max(exp->MaxArgs(), tail->MaxArgs());
}
int A::PairExpList::ExpNumber() const { return 1 + tail->ExpNumber(); }

IntAndTable *A::PairExpList::Interp(Table *table, std::list<int> &list) const {
  IntAndTable *intAndTable = exp->Interp(table);
  list.push_back(intAndTable->i);
  return tail->Interp(intAndTable->t, list);
}

int A::LastExpList::MaxArgs() const { return exp->MaxArgs(); }

int A::LastExpList::ExpNumber() const { return 1; }

IntAndTable *A::LastExpList::Interp(Table *table, std::list<int> &list) const {
  IntAndTable *intAndTable = exp->Interp(table);
  list.push_back(intAndTable->i);
  return intAndTable;
}

int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
} // namespace A
