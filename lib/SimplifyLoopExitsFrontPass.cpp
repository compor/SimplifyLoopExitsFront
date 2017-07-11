//
//
//

#include "Config.hpp"

#include "Utils.hpp"

#include "SimplifyLoopExitsFrontPass.hpp"

#if SIMPLIFYLOOPEXITSFRONT_USES_ANNOTATELOOPS
#include "AnnotateLoops.hpp"
#endif // SIMPLIFYLOOPEXITSFRONT_USES_ANNOTATELOOPS

#if SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS
#include "SimplifyLoopExits.hpp"
#endif // SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS

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
// using char llvm::LowerSwitchID

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::list
// using llvm::cl::desc
// using llvm::cl::value_desc
// using llvm::cl::location
// using llvm::cl::ZeroOrMore

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include "llvm/IR/Verifier.h"
// using llvm::verifyFunction

#include <string>
// using std::string
// using std::stoul

#include <fstream>
// using std::ifstream

#include <set>
// using std::set

#include <exception>
// using std::terminate

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

static llvm::cl::opt<unsigned int>
    LoopDepthLB("slef-loop-depth-lb",
                llvm::cl::desc("loop depth lower bound (inclusive)"),
                llvm::cl::init(1u));

static llvm::cl::opt<unsigned int>
    LoopDepthUB("slef-loop-depth-ub",
                llvm::cl::desc("loop depth upper bound (inclusive)"),
                llvm::cl::init(std::numeric_limits<unsigned>::max()));

static llvm::cl::opt<unsigned int> LoopExitingBlockDepthLB(
    "slef-loop-exiting-block-depth-lb",
    llvm::cl::desc("loop exiting block depth lower bound (inclusive)"),
    llvm::cl::init(1u));

static llvm::cl::opt<unsigned int> LoopExitingBlockDepthUB(
    "slef-loop-exiting-block-depth-ub",
    llvm::cl::desc("loop exiting block depth upper bound (inclusive)"),
    llvm::cl::init(std::numeric_limits<unsigned>::max()));

static llvm::cl::list<unsigned int>
    LoopIDWhiteList("slef-loop-id",
                    llvm::cl::desc("Specify loop ids to whitelist"),
                    llvm::cl::value_desc("loop id"), llvm::cl::ZeroOrMore);

static llvm::cl::opt<std::string>
    LoopIDWhiteListFilename("slef-loop-id-whitelist",
                            llvm::cl::desc("loop id whitelist filename"));

#if SIMPLIFYLOOPEXITSFRONT_DEBUG
bool passDebugFlag = false;
static llvm::cl::opt<bool, true>
    Debug("slef-debug", llvm::cl::desc("debug simplify loop exits front pass"),
          llvm::cl::location(passDebugFlag));
#endif // SIMPLIFYLOOPEXITSFRONT_DEBUG

namespace {

void checkCmdLineOptions(void) {
  assert(LoopDepthLB && LoopDepthUB && LoopExitingBlockDepthLB &&
         LoopExitingBlockDepthUB && "Loop depth bounds cannot be zero!");

  assert(LoopDepthLB <= LoopDepthUB &&
         "Loop depth lower bound cannot be greater that upper!");

  assert(LoopExitingBlockDepthLB <= LoopExitingBlockDepthUB &&
         "Loop exiting block depth lower bound cannot be greater that upper!");

  return;
}

} // namespace anonymous end

//

bool SimplifyLoopExitsFrontPass::runOnModule(llvm::Module &M) {
  bool hasModuleChanged = false;
  bool useLoopIDWhitelist = !LoopIDWhiteListFilename.empty();
  llvm::SmallVector<llvm::Loop *, 16> workList;
  std::set<unsigned> loopIDs;

  if (useLoopIDWhitelist) {
    std::ifstream loopIDWhiteListFile{LoopIDWhiteListFilename};

    if (loopIDWhiteListFile.is_open()) {
      std::string loopID;

      while (loopIDWhiteListFile >> loopID) {
        if (loopID.size() > 0 && loopID[0] != '#')
          loopIDs.insert(std::stoul(loopID));
      }

      loopIDWhiteListFile.close();
    } else
      llvm::errs() << "could not open file: \'" << LoopIDWhiteListFilename
                   << "\'\n";
  }

  for (const auto &e : LoopIDWhiteList)
    loopIDs.insert(e);

  for (auto &CurFunc : M) {
    if (CurFunc.isDeclaration())
      continue;

    auto &DT =
        getAnalysis<llvm::DominatorTreeWrapperPass>(CurFunc).getDomTree();
    auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>(CurFunc).getLoopInfo();

    workList.clear();

#if SIMPLIFYLOOPEXITSFRONT_USES_ANNOTATELOOPS
    AnnotateLoops al;

    for (auto *e : LI)
      if (al.hasAnnotatedId(*e)) {
        auto id = al.getAnnotatedId(*e);
        if (loopIDs.count(id))
          workList.push_back(e);
      }
#endif // SIMPLIFYLOOPEXITSFRONT_USES_ANNOTATELOOPS

    for (auto i = 0; i < workList.size(); ++i)
      for (auto &e : workList[i]->getSubLoops())
        workList.push_back(e);

    workList.erase(
        std::remove_if(workList.begin(), workList.end(), [](const auto *e) {
          auto d = e->getLoopDepth();
          return d < LoopDepthLB || d > LoopDepthUB;
        }), workList.end());

    // remove any loops that their exiting blocks are outside of the
    // specified loop next levels
    workList.erase(
        std::remove_if(workList.begin(), workList.end(), [&LI](const auto *e) {
          llvm::SmallVector<llvm::BasicBlock *, 5> exiting;
          e->getExitingBlocks(exiting);

          return std::any_of(exiting.begin(), exiting.end(),
                             [&LI](const auto *x) {
                               auto d = LI[x]->getLoopDepth();
                               return d < LoopExitingBlockDepthLB ||
                                      d > LoopExitingBlockDepthUB;
                             });
        }), workList.end());

    std::reverse(workList.begin(), workList.end());

#if SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS
    workList.erase(
        std::remove_if(workList.begin(), workList.end(), [](const auto *e) {
          return isLoopExitSimplifyForm(*e);
        }), workList.end());

    SimplifyLoopExits sle;

    for (auto &e : workList) {
      auto id = al.getAnnotatedId(*e);
      llvm::errs() << "processing loop id: " << id << "\n";
      bool changed = sle.transform(*e, LI, &DT);

      hasModuleChanged |= changed;
      if (changed)
        llvm::errs() << "transformed loop id: " << id << "\n";

      bool isVerified = !llvm::verifyFunction(CurFunc, &llvm::errs());
      if (!isVerified) {
        llvm::errs() << "terminating!\n";
        std::terminate();
      }
    }
#endif // SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS
  }

  return hasModuleChanged;
}

void SimplifyLoopExitsFrontPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
#if SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS
  AU.addPreservedID(llvm::LoopSimplifyID);
  AU.addRequiredTransitiveID(llvm::LowerSwitchID);
  AU.addRequiredTransitiveID(llvm::LoopSimplifyID);
#endif // SIMPLIFYLOOPEXITSFRONT_USES_SIMPLIFYLOOPEXITS
  AU.addRequiredTransitive<llvm::DominatorTreeWrapperPass>();
  AU.addPreserved<llvm::DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addPreserved<llvm::LoopInfoWrapperPass>();

  return;
}

} // namespace icsa end
