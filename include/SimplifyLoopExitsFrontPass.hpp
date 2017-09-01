//
//
//

#ifndef SIMPLIFYLOOPEXITSFRONTPASS_HPP
#define SIMPLIFYLOOPEXITSFRONTPASS_HPP

#include "llvm/Pass.h"
// using llvm::ModulePass

namespace llvm {
class Module;
} // namespace llvm end

namespace icsa {

class SimplifyLoopExitsFrontPass : public llvm::ModulePass {
public:
  static char ID;

  SimplifyLoopExitsFrontPass() : llvm::ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // header
