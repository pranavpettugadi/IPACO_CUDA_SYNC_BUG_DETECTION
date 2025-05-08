#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <string>
#include <vector>

using namespace llvm;
using namespace std;

namespace {
struct sync_bug_detect : public FunctionPass {
  static char ID;
  sync_bug_detect() : FunctionPass(ID) {}

  using AccessMap = map<string, vector<pair<string, string>>>;
  vector<AccessMap> regions;

  string getName(Value *V) {
    string s;
    raw_string_ostream rso(s);
    V->printAsOperand(rso, false);
    return rso.str();
  }

  void detectConflicts() {
    for (size_t i = 1; i < regions.size(); ++i) {
      const auto &prev = regions[i - 1];
      const auto &curr = regions[i];

      errs() << "\n--- Comparing Region #" << i - 1 << " and #" << i << " ---\n";
      bool foundConflict = false;

      for (const auto &[var, prevAccesses] : prev) {
        auto it = curr.find(var);
        if (it == curr.end())
          continue;

        const auto &currAccesses = it->second;
        // get last access from prevaccesses and first access from curraccesses

        string lastAccess = prevAccesses.back().first;
        string firstAccess = currAccesses.front().first;
        string lastType = prevAccesses.back().second;
        string firstType = currAccesses.front().second;

        // Check for potential hazards
        if(( lastType=="read" && firstType=="read") || lastAccess == firstAccess ) {
          errs() << "Potential hazard detected on " << var << "[" << lastAccess << "] and " << var << "[" << firstAccess << "]\n";
          foundConflict = true;
          break;
        }

      }
    }
  }

  bool runOnFunction(Function &F) override {
    AccessMap current;

    for (auto &BB : F) {
      for (auto &I : BB) {
        if (auto *call = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = call->getCalledFunction();
          if (calledFunc && calledFunc->getName().contains("llvm.nvvm.barrier0")) {
            regions.push_back(current);
            current.clear();
            continue;
          }
        }

        Value *ptr = nullptr;
        string type;

        if (auto *load = dyn_cast<LoadInst>(&I)) {
          ptr = load->getPointerOperand();
          type = "read";
        } else if (auto *store = dyn_cast<StoreInst>(&I)) {
          ptr = store->getPointerOperand();
          type = "write";
        }

        if (!ptr)
          continue;

        string baseName, indexStr = "unknown";

        if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
          Value *base = gep->getPointerOperand();
          baseName = getName(base);

          if (gep->getNumIndices() >= 1) {
            Value *index = gep->getOperand(1);
            indexStr = getName(index);
          }
          current[baseName].push_back({indexStr, type});
        } 


        
      }
    }

    if (!current.empty())
      regions.push_back(current);

    for (size_t i = 0; i < regions.size(); ++i) {
      errs() << "\n=== Region between __syncthreads() #" << i - 1 << " and #" << i << " ===\n";
      for (const auto &[var, accesses] : regions[i]) {
        errs() << "Variable: " << var << "\n";
        errs() << "  Accesses:\n";
        for (const auto &[idx, type] : accesses)
          errs() << "    " << type << " at index " << idx << "\n";
      }
    }

    detectConflicts();
    return false;
  }
};
} // namespace

char sync_bug_detect::ID = 0;
static RegisterPass<sync_bug_detect>
    X("sync_bug_detect", "Detects and Reports Shared Memory Accesses and Potential Barrier Issues");

