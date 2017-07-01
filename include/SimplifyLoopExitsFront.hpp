//
//
//

#ifndef SIMPLIFYLOOPEXITSFRONT_HPP
#define SIMPLIFYLOOPEXITSFRONT_HPP

#include "llvm/Pass.h"
// using llvm::ModulePass

namespace llvm {
class Module;
} // namespace llvm end

namespace icsa {

class SimplifyLoopExitsFront : public llvm::ModulePass {
public:
  static char ID;

  SimplifyLoopExitsFront() : llvm::ModulePass(ID) {}

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnModule(llvm::Module &M) override;
};

} // namespace icsa end

#endif // SIMPLIFYLOOPEXITSFRONT_HPP
