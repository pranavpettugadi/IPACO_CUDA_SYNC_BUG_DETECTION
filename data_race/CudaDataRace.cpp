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

/*
// CudaDataRace.cpp - CUDA Data Race Detector Pass
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <bits/stdc++.h>

using namespace llvm;
using namespace std;

namespace {

struct MemAccess {
  string arrayName;
  int coeff; // coefficient of tid (thread index)
  int val;  // constant value computed by tid = 0
  Instruction *inst; // pointer to the original instruction
  Value *ptrOperand; // store instruction pointer operand
};

struct CudaDataRace : PassInfoMixin<CudaDataRace> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
    errs() << "Running the pass on function: " << F.getName() << "\n";

    unordered_map<Value*, int> tidCoeffMap;
    
    // Initialize %tid.x, %tid.y, %tid.z to 1 if found
    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *call = dyn_cast<CallInst>(&I)) {
          if (Function *callee = call->getCalledFunction()) {
            StringRef name = callee->getName();
            if (name.contains("llvm.nvvm.read.ptx.sreg.tid.x") ||
                name.contains("llvm.nvvm.read.ptx.sreg.tid.y") ||
                name.contains("llvm.nvvm.read.ptx.sreg.tid.z")) {
              tidCoeffMap[&I] = 1; // treat all thread IDs as having coefficient 1
            }
          }
        }
      }
    }

    // Track variable -> coefficient of tid
    for (auto &BB : F) {
      for (auto &I : BB) {

        // Propagate coefficients through binary operations
        if (auto *binOp = dyn_cast<BinaryOperator>(&I)) {
          Value *op1 = binOp->getOperand(0);
          Value *op2 = binOp->getOperand(1);
          int coeff1 = tidCoeffMap.count(op1) ? tidCoeffMap[op1] : 0;
          int coeff2 = tidCoeffMap.count(op2) ? tidCoeffMap[op2] : 0;

          switch (binOp->getOpcode()) {
            case Instruction::Add:
              tidCoeffMap[&I] = coeff1 + coeff2;
              break;
            case Instruction::Sub:
              tidCoeffMap[&I] = coeff1 - coeff2; 
              break;

            case Instruction::Mul:
              if (coeff1 != 0 && isa<ConstantInt>(op2)) {
                int constVal = cast<ConstantInt>(op2)->getSExtValue();
                tidCoeffMap[&I] = coeff1 * constVal;
              } else if (coeff2 != 0 && isa<ConstantInt>(op1)) {
                int constVal = cast<ConstantInt>(op1)->getSExtValue();
                tidCoeffMap[&I] = coeff2 * constVal;
              } else {
                tidCoeffMap[&I] = 0; // complex, not a tid expression
              }
              break;

            default:
              tidCoeffMap[&I] = 0;
          }
        }

        // Step 3: Analyze store instructions that use tidCoeffMap
        if (auto *store = dyn_cast<StoreInst>(&I)) {
          Value *ptr = store->getPointerOperand();
          analyzePointer(ptr, store, tidCoeffMap);
        }
      }
    }

    return PreservedAnalyses::all(); // This is an analysis pass
  }

  void analyzePointer(Value *ptr, Instruction *context, const std::unordered_map<Value*, int> &tidCoeffMap) {
    if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
      if (gep->getNumOperands() < 2) return;

      // Step 4: Check if index is dependent on threadIdx.x
      Value *index = gep->getOperand(1); // assume 1D array
      int coeff = tidCoeffMap.count(index) ? tidCoeffMap.at(index) : 0;

      if (coeff != 0) {
        // A potential data race - output the store instruction and its coefficient
        errs() << "Possible data race at store instruction: ";
        context->print(errs());
        errs() << " (tid coefficient = " << coeff << ")\n";
      }
    }
  }
};
} //namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "cuda-data-race", "21.0.0git",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          if (Name == "cuda-data-race") {
            FPM.addPass(CudaDataRace());
            return true;
          }
          return false;
        });
    }
  };
}
*/
