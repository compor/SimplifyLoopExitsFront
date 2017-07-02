//
//
//

#include "Config.hpp"

#include "Utils.hpp"

#include "SimplifyLoopExitsFrontPass.hpp"

#include "AnnotateLoops.hpp"

#include "SimplifyLoopExits.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Module.h"
// using llvm::Module

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopInfoWrapperPass
// using llvm::LoopInfo

#include "llvm/IR/Dominators.h"
// using llvm::DominatorTree
// using llvm::DominatorTreeWrapperPass

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Transforms/Scalar.h"
// using char llvm::LoopInfoSimplifyID

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#define DEBUG_TYPE "simplify_loop_exits_front"

#define STRINGIFY_UTIL(x) #x
#define STRINGIFY(x) STRINGIFY_UTIL(x)

#define PRJ_CMDLINE_DESC(x) x " (version: " STRINGIFY(VERSION_STRING) ")"

namespace icsa {

// plugin registration for opt

char SimplifyLoopExitsFrontPass::ID = 0;
static llvm::RegisterPass<SimplifyLoopExitsFrontPass>
    X("simplify-loop-exits-front",
      PRJ_CMDLINE_DESC("simplify loop exits front pass"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerSimplifyLoopExitsFrontPass(const llvm::PassManagerBuilder &Builder,
                                   llvm::legacy::PassManagerBase &PM) {
  PM.add(new SimplifyLoopExitsFrontPass());

  return;
}

static llvm::RegisterStandardPasses RegisterSimplifyLoopExitsFrontPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerSimplifyLoopExitsFrontPass);

//

llvm::cl::list<unsigned int>
    LoopIDWhileList("slef-loop-id",
                    llvm::cl::desc("Specify loop ids to whitelist"),
                    llvm::cl::value_desc("loop id"), llvm::cl::OneOrMore);

static llvm::cl::opt<std::string>
    LoopIDWhiteListFilename("slef-loop-id-whitelist",
                            llvm::cl::desc("loop id whitelist filename"));

#if SIMPLIFYLOOPEXITSFRONT_DEBUG
bool passDebugFlag = false;
static llvm::cl::opt<bool, true>
    Debug("slef-debug", llvm::cl::desc("debug simplify loop exits front pass"),
          llvm::cl::location(passDebugFlag));
#endif // SIMPLIFYLOOPEXITSFRONT_DEBUG

//

bool SimplifyLoopExitsFrontPass::runOnModule(llvm::Module &M) {
  bool hasModuleChanged = false;
  llvm::SmallVector<llvm::Loop *, 16> workList;
  SimplifyLoopExits sle;
  AnnotateLoops al;

  for (auto &CurFunc : M) {
    if (CurFunc.isDeclaration())
      continue;

    auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>(CurFunc).getLoopInfo();
    auto &DT =
        getAnalysis<llvm::DominatorTreeWrapperPass>(CurFunc).getDomTree();

    workList.clear();
    workList.append(&*(LI.begin()), &*(LI.end()));

    for (auto i = 0; i < workList.size(); ++i)
      for (auto &e : workList[i]->getSubLoops())
        if (al.hasAnnotatedId(*e))
          workList.push_back(e);
  }

  return false;
}

void SimplifyLoopExitsFrontPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<llvm::DominatorTreeWrapperPass>();
  AU.addPreserved<llvm::DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addPreserved<llvm::LoopInfoWrapperPass>();
  AU.addRequiredTransitiveID(llvm::LoopSimplifyID);
  AU.addPreservedID(llvm::LoopSimplifyID);

  return;
}

} // namespace icsa end
