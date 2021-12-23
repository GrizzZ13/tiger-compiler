#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include <list>
#include <memory>

#include "tiger/absyn/absyn.h"
#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/frame.h"
#include "tiger/semant/types.h"

namespace tr {

class Exp;
class ExpAndTy;
class Level;

class Access {
public:
  Level *level_;
  frame::Access *access_;

  Access(Level *level, frame::Access *access)
      : level_(level), access_(access) {}
  static Access *AllocLocal(Level *level, bool escape);
};

class Level {
public:
  frame::Frame *frame_;
  Level *parent_;

  /* TODO: Put your lab5 code here */
  Level(frame::Frame *frame, Level *parent): frame_(frame), parent_(parent) {}
  std::list<tr::Access*> GetFormals(){
    std::list<tr::Access*> accessList;
    std::list<frame::Access*> faccessList = frame_->GetFormals();
    for(auto & faccess : faccessList){
      accessList.push_back(new tr::Access(this, faccess));
    }
    return accessList;
  }
  static Level* NewLevel(Level *parent, temp::Label *name, std::list<bool>& escapes);
};

class ProgTr {
public:
  /* TODO: Put your lab5 code here */ 
  ProgTr(std::unique_ptr<absyn::AbsynTree> absyn_tree, std::unique_ptr<err::ErrorMsg> errormsg):
  tenv_(std::make_unique<env::TEnv>()), venv_(std::make_unique<env::VEnv>())
  {
    absyn_tree_ = std::move(absyn_tree);
    errormsg_ = std::move(errormsg);
  }

  /**
   * Translate IR tree
   */
  void Translate();

  /**
   * Transfer the ownership of errormsg to outer scope
   * @return unique pointer to errormsg
   */
  std::unique_ptr<err::ErrorMsg> TransferErrormsg() {
    return std::move(errormsg_);
  }


private:
  std::unique_ptr<absyn::AbsynTree> absyn_tree_;
  std::unique_ptr<err::ErrorMsg> errormsg_;
  std::unique_ptr<Level> main_level_;
  std::unique_ptr<env::TEnv> tenv_;
  std::unique_ptr<env::VEnv> venv_;

  // Fill base symbol for var env and type env
  void FillBaseVEnv();
  void FillBaseTEnv();
};

} // namespace tr

#endif
