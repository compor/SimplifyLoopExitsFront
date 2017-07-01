//
//
//

#ifndef SIMPLIFYLOOPEXITSFRONT_HPP
#define SIMPLIFYLOOPEXITSFRONT_HPP

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm end

namespace {

class SimplifyLoopExitsFront : public llvm::FunctionPass {
public:
  static char ID;

  SimplifyLoopExitsFront() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &f) override;
};

} // namespace unnamed end

#endif // SIMPLIFYLOOPEXITSFRONT_HPP
