//
//
//

#include "Config.hpp"

#include "Utils.hpp"

#include "SimplifyLoopExitsFront.hpp"

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

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

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

char SimplifyLoopExitsFront::ID = 0;
static llvm::RegisterPass<SimplifyLoopExitsFront>
    X("simplify-loop-exits-front",
      PRJ_CMDLINE_DESC("simplify loop exits front pass"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerSimplifyLoopExitsFront(const llvm::PassManagerBuilder &Builder,
                               llvm::legacy::PassManagerBase &PM) {
  PM.add(new SimplifyLoopExitsFront());

  return;
}

static llvm::RegisterStandardPasses
    RegisterSimplifyLoopExitsFront(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                                   registerSimplifyLoopExitsFront);

//

#if SIMPLIFYLOOPEXITSFRONT_DEBUG
bool passDebugFlag = false;
static llvm::cl::opt<bool, true>
    Debug("slef-debug", llvm::cl::desc("debug simplify loop exits front pass"),
          llvm::cl::location(passDebugFlag));
#endif // SIMPLIFYLOOPEXITSFRONT_DEBUG

//

bool SimplifyLoopExitsFront::runOnModule(llvm::Module &M) { return false; }

} // namespace icsa end
