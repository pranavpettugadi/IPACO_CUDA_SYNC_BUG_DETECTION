#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
using namespace std;

namespace {
  struct CudaDataRace : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    CudaDataRace() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      errs() << "CudaDataRace: ";
      errs().write_escaped(F.getName()) << '\n';
      return false;
    }
  };
}

char CudaDataRace::ID = 0;
static RegisterPass<CudaDataRace> X("cuda-data-race", "cuda data race pass");
