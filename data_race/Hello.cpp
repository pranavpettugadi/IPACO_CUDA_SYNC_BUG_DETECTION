#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <bits/stdc++.h>
#include <map>
#include <string>
#include <vector>

#include "llvm/IR/Module.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace std;

namespace {
struct Hello : public FunctionPass {
  static char ID;
  Hello()
      : FunctionPass(ID), File("temp.txt", EC, llvm::sys::fs::OF_Text),
        outfile("output.txt", EC, llvm::sys::fs::OF_Text) {}

  /* Constant Prop Data Structures*/
  map<BasicBlock *, map<Value *, Constant *>> bb_var_values;
  map<BasicBlock *, map<Value *, string>> bb_lattice;
  std::error_code EC;
  llvm::raw_fd_ostream File; // Declare the file stream here
  llvm::raw_fd_ostream outfile;
  string Input_File;
  map<Value *, Constant *>
      last_block_var_values; // to store the last block values

  /* Cuda Bug Detection Data Structures */
  map<BasicBlock *, vector<BasicBlock *>> bb_predecessors;
  map<string, vector<string>> var_dep_on_tid;
  map<BasicBlock *, string> block_name;
  map<BasicBlock *, Instruction *> branchInst_per_block;
  pair<string, string> b1_b2;
  pair<string, string> a1_a2;

  /* Constant Prop Functions */
  void remove_first_line_of_file() {
    ifstream fin("temp.txt");

    // remove last 3 characters from the Input_File
    Input_File.pop_back();
    Input_File.pop_back();
    Input_File.pop_back();

    Input_File += ".txt";
    ofstream fout("./output/" + Input_File);

    string line;
    int count = 0;
    while (getline(fin, line)) {
      if (count == 0) {
        count++;
        continue;
      }
      fout << line << endl;
    }
    fout << "}\n";
    fin.close();
    fout.close();
    //   // delete temp.txt file
    //   remove("temp.txt");

    // delete content from temp.txt
    ofstream ofs;
    ofs.open("temp.txt", std::ofstream::out | std::ofstream::trunc);
    ofs.close();
  }

  string getRegisterName_Load(Instruction &I) {
    // for load instruction
    string load_inst;
    raw_string_ostream rso(load_inst);
    I.print(rso);
    string reg_name;
    for (auto ch : load_inst) {
      if (ch == '%') {
        reg_name = "";
      } else if (ch == '=') {
        break;
      } else {
        reg_name += ch;
      }
    }
    reg_name.pop_back();

    return reg_name;
  }

  vector<string> get_var_names(Instruction &I) {
    vector<string> var_names;
    string load_inst;
    raw_string_ostream rso(load_inst);
    I.print(rso);
    string temp;
    for (long unsigned int i = 0; i < load_inst.size(); i++) {
      if (load_inst[i] == '%') {
        temp = "";
        i++;
        while (i != load_inst.size() and
               (isalpha(load_inst[i]) or isdigit(load_inst[i]))) {
          temp += load_inst[i];
          i++;
        }
        var_names.push_back(temp);
      }
    }
    return var_names;
  }

  set<Constant *> get_prev_values(vector<BasicBlock *> preds, Value *val) {
    set<Constant *> prev_values;
    for (BasicBlock *pred : preds) {
      if (bb_var_values.find(pred) != bb_var_values.end()) {
        if (bb_var_values[pred][val] != NULL) {
          prev_values.insert(bb_var_values[pred][val]);
        }
      }
    }
    return prev_values;
  }

  void print_val(map<Value *, Constant *> &temp_var_values,
                 map<Value *, string> &temp_lattice, Value *op) {
    if (temp_lattice[op] == "BOTTOM") {
      File << "BOTTOM";
    } else if (temp_lattice[op] == "CONSTANT") {
      File << temp_var_values[op]->getUniqueInteger();
    } else {
      File << "TOP";
    }
  }

  void AllocaInst_func(Instruction &I,
                       map<Value *, Constant *> &temp_var_values,
                       map<Value *, string> &temp_lattice) {
    File << I;
    AllocaInst *allocaInst = dyn_cast<AllocaInst>(&I);
    if (!allocaInst)
      return; // Ensure casting was successful

    string varName = allocaInst->getName().str();
    temp_var_values[allocaInst] = NULL;
    temp_lattice[allocaInst] = "TOP";
    // // print varname
    File << " --> %" << varName << "=TOP\n";
  }

  void StoreInst_func(Instruction &I, map<Value *, Constant *> &temp_var_values,
                      map<Value *, string> &temp_lattice,
                      vector<BasicBlock *> preds) {
    File << I;
    StoreInst *storeInst = dyn_cast<StoreInst>(&I);
    Value *stored_value = storeInst->getValueOperand();
    Value *pointer = storeInst->getPointerOperand();

    vector<string> var_names = get_var_names(I);

    if (Constant *const_val = dyn_cast<Constant>(
            stored_value)) { // if the stored value is a constant
      ConstantInt *const_val_int = (ConstantInt *)const_val;
      temp_var_values[pointer] = const_val;
      temp_lattice[pointer] = "CONSTANT";

      File << " --> %" << var_names[0] << "="
           << const_val_int->getUniqueInteger() << "\n";

    }

    else { // if the stored value is a variable
      vector<string> var_names = get_var_names(I);
      if (temp_lattice[stored_value] == "BOTTOM") {
        temp_lattice[pointer] = "BOTTOM";
        temp_var_values[pointer] = NULL;

        File << " --> %" << var_names[0] << "=BOTTOM, ";
        File << "%" << var_names[1] << "=BOTTOM\n";
      } else if (temp_lattice[stored_value] == "CONSTANT") {
        temp_lattice[pointer] = "CONSTANT";
        temp_var_values[pointer] = temp_var_values[stored_value];

        File << " --> %" << var_names[0] << "="
             << (temp_var_values[pointer])->getUniqueInteger() << ", ";
        File << "%" << var_names[1] << "="
             << (temp_var_values[pointer])->getUniqueInteger() << "\n";
      } else if (temp_lattice[stored_value] == "TOP") {
        temp_lattice[pointer] = "TOP";
        temp_var_values[pointer] = NULL;
      }
    }
  }

  void LoadInst_func(Instruction &I, map<Value *, Constant *> &temp_var_values,
                     map<Value *, string> &temp_lattice,
                     vector<BasicBlock *> preds) {

    LoadInst *loadInst = dyn_cast<LoadInst>(&I);
    Value *pointer = loadInst->getPointerOperand();

    if (Constant *const_val =
            dyn_cast<Constant>(pointer)) { // if the stored value is a constant
      ConstantInt *const_val_int = (ConstantInt *)const_val;
      temp_var_values[loadInst] = const_val;

      const APInt temp = const_val_int->getUniqueInteger();
      temp_lattice[loadInst] = "CONSTANT";
      temp_lattice[pointer] = "CONSTANT";

    }

    else { // if the stored value is a variable. Eg:- %1 = load i32, i32* %x
      vector<string> var_names = get_var_names(I);
      if (temp_lattice[pointer] == "BOTTOM") {
        temp_lattice[loadInst] = "BOTTOM";
        temp_var_values[loadInst] = NULL;

      } else if (temp_lattice[pointer] == "CONSTANT") {
        temp_lattice[loadInst] = "CONSTANT";
        temp_var_values[loadInst] = temp_var_values[pointer];

      } else if (temp_lattice[pointer] == "TOP") {
        temp_lattice[loadInst] = "TOP";
        temp_var_values[loadInst] = NULL;
      }
    }
  }

  void BinaryOperator_func(Instruction &I, BinaryOperator *BO,
                           map<Value *, Constant *> &temp_var_values,
                           map<Value *, string> &temp_lattice,
                           vector<BasicBlock *> preds) {

    Value *op1 = BO->getOperand(0);
    Value *op2 = BO->getOperand(1);

    ConstantInt *op1_const_int = dyn_cast<ConstantInt>(op1);
    if (op1_const_int) {
      op1_const_int = dyn_cast<ConstantInt>(op1);
      temp_var_values[op1] = op1_const_int;
      temp_lattice[op1] = "CONSTANT";
    }

    ConstantInt *op2_const_int = dyn_cast<ConstantInt>(op2);
    if (op2_const_int) {
      op2_const_int = dyn_cast<ConstantInt>(op2);
      temp_var_values[op2] = op2_const_int;
      temp_lattice[op2] = "CONSTANT";
    }

    vector<string> var_names = get_var_names(I);
    if (temp_lattice[op1] == "BOTTOM" || temp_lattice[op2] == "BOTTOM") {
      temp_lattice[&I] = "BOTTOM";
      temp_var_values[&I] = NULL;
      if (!dyn_cast<Constant>(op1) && !dyn_cast<Constant>(op2)) { // 2 variables

        File << "%" << var_names[1] << "=";
        print_val(temp_var_values, temp_lattice, op1);
        File << ", ";
        File << "%" << var_names[2] << "=";
        print_val(temp_var_values, temp_lattice, op2);
        File << "\n";
      } else { // 1 variable
        File << "%" << var_names[1] << "=BOTTOM\n";
      }
    } else {
      temp_lattice[&I] = "CONSTANT";
      APInt result(32, 0, false);
      if (BO->getOpcode() == Instruction::Add) {
        result = temp_var_values[op1]->getUniqueInteger() +
                 temp_var_values[op2]->getUniqueInteger();
      } else if (BO->getOpcode() == Instruction::FAdd) {
        result = temp_var_values[op1]->getUniqueInteger() +
                 temp_var_values[op2]->getUniqueInteger();
      } else if (BO->getOpcode() == Instruction::Sub) {
        result = temp_var_values[op1]->getUniqueInteger() -
                 temp_var_values[op2]->getUniqueInteger();
      } else if (BO->getOpcode() == Instruction::Mul) {
        result = temp_var_values[op1]->getUniqueInteger() *
                 temp_var_values[op2]->getUniqueInteger();
      } else if (BO->getOpcode() == Instruction::SDiv) {
        result = temp_var_values[op1]->getUniqueInteger().sdiv(
            temp_var_values[op2]->getUniqueInteger());
      } else if (BO->getOpcode() == Instruction::SRem) {
        result = temp_var_values[op1]->getUniqueInteger().srem(
            temp_var_values[op2]->getUniqueInteger());
      }

      temp_var_values[&I] = ConstantInt::get(I.getContext(), result);
      File << " --> %" << var_names[0] << "=" << result << ", ";
      if (!dyn_cast<Constant>(op1) && !dyn_cast<Constant>(op2)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op1]->getUniqueInteger() << ", ";
        File << "%" << var_names[2] << "="
             << temp_var_values[op2]->getUniqueInteger() << "\n";
      } else if (!dyn_cast<Constant>(op1)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op1]->getUniqueInteger() << "\n";
      } else if (!dyn_cast<Constant>(op2)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op2]->getUniqueInteger() << "\n";
      } else {
        File << "\n";
      }
    }
    File << "\n";
  }

  void ComparisonOperator_func(Instruction &I,
                               map<Value *, Constant *> &temp_var_values,
                               map<Value *, string> &temp_lattice,
                               vector<BasicBlock *> preds) {
    File << I;
    ICmpInst *icmpInst = dyn_cast<ICmpInst>(&I);
    Value *op1 = icmpInst->getOperand(0);
    Value *op2 = icmpInst->getOperand(1);
    ConstantInt *op1_const_int = dyn_cast<ConstantInt>(op1);
    if (op1_const_int) {
      op1_const_int = dyn_cast<ConstantInt>(op1);
      temp_var_values[op1] = op1_const_int;
      temp_lattice[op1] = "CONSTANT";
    }

    ConstantInt *op2_const_int = dyn_cast<ConstantInt>(op2);
    if (op2_const_int) {
      op2_const_int = dyn_cast<ConstantInt>(op2);
      temp_var_values[op2] = op2_const_int;
      temp_lattice[op2] = "CONSTANT";
    }

    vector<string> var_names = get_var_names(I);
    if (temp_lattice[op1] == "BOTTOM" || temp_lattice[op2] == "BOTTOM") {
      temp_lattice[&I] = "BOTTOM";
      temp_var_values[&I] = NULL;
      File << " --> %" << var_names[0] << "=BOTTOM, ";
      if (!dyn_cast<Constant>(op1) && !dyn_cast<Constant>(op2)) {
        print_val(temp_var_values, temp_lattice, op1);

        print_val(temp_var_values, temp_lattice, op2);

        File << "%" << var_names[1] << "=";
        print_val(temp_var_values, temp_lattice, op1);
        File << ", ";
        File << "%" << var_names[2] << "=";
        print_val(temp_var_values, temp_lattice, op2);
        File << "\n";
      } else {
        File << "%" << var_names[1] << "=BOTTOM\n";
      }
    } else {
      temp_lattice[&I] = "CONSTANT";
      APInt result(2, 0, false);
      if (icmpInst->getPredicate() == CmpInst::ICMP_EQ) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger() ==
                           temp_var_values[op2]->getUniqueInteger(),
                       false);
      } else if (icmpInst->getPredicate() == CmpInst::ICMP_NE) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger() !=
                           temp_var_values[op2]->getUniqueInteger(),
                       false);
      } else if (icmpInst->getPredicate() == CmpInst::ICMP_SGT) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger().sgt(
                           temp_var_values[op2]->getUniqueInteger()),
                       false);
      } else if (icmpInst->getPredicate() == CmpInst::ICMP_SGE) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger().sge(
                           temp_var_values[op2]->getUniqueInteger()),
                       false);
      } else if (icmpInst->getPredicate() == CmpInst::ICMP_SLT) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger().slt(
                           temp_var_values[op2]->getUniqueInteger()),
                       false);
      } else if (icmpInst->getPredicate() == CmpInst::ICMP_SLE) {
        result = APInt(2,
                       temp_var_values[op1]->getUniqueInteger().sle(
                           temp_var_values[op2]->getUniqueInteger()),
                       false);
      }

      temp_var_values[&I] = ConstantInt::get(I.getContext(), result);
      File << " --> %" << var_names[0] << "=" << result << ", ";
      if (!dyn_cast<Constant>(op1) && !dyn_cast<Constant>(op2)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op1]->getUniqueInteger() << ", ";
        File << "%" << var_names[2] << "="
             << temp_var_values[op2]->getUniqueInteger() << "\n";
      } else if (!dyn_cast<Constant>(op1)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op1]->getUniqueInteger() << "\n";
      } else if (!dyn_cast<Constant>(op2)) {

        File << "%" << var_names[1] << "="
             << temp_var_values[op2]->getUniqueInteger() << "\n";
      } else {
        File << "\n";
      }
    }
  }

  void RetOperator_func(Instruction &I, map<Value *, Constant *> &var_values,
                        map<Value *, string> &lattice) {
    File << I;

    ReturnInst *retInst = dyn_cast<ReturnInst>(&I);
    if (!retInst)
      return;

    Value *retVal = retInst->getReturnValue();

    // Handle 'ret void'
    if (!retVal) {
      File << "\n";
      return;
    }

    vector<string> var_names = get_var_names(I);
    Constant *const_val = dyn_cast<Constant>(retVal);

    if (const_val != nullptr) {
      File << "\n";
    } else {
      if (lattice[retVal] == "BOTTOM") {
        File << " --> %" << var_names[0] << "=BOTTOM\n";
      } else {

        File << " --> %" << var_names[0] << "="
             << var_values[retVal]->getUniqueInteger() << "\n";
      }
    }
  }

  void CallInst_func(Instruction &I, map<Value *, Constant *> &var_values,
                     map<Value *, string> &lattice, vector<BasicBlock *> preds,
                     int block_id, int block_dim, int thread_id) {
    File << I;
    CallInst *callInst = dyn_cast<CallInst>(&I);
    Function *calledFunction = callInst->getCalledFunction();
    vector<string> var_names = get_var_names(I);

    if (calledFunction->getName() == "printf") {
      vector<Value *> operands;
      for (unsigned int i = 1; i < callInst->getNumOperands() - 1; i++) {
        operands.push_back(callInst->getOperand(i));
      }

      for (unsigned int i = 0; i < operands.size(); i++) {
        // if the operand is a constant just print the value
        if (Constant *const_val = dyn_cast<Constant>(operands[i])) {

          File << " --> %" << var_names[i + 1] << "="
               << const_val->getUniqueInteger();
        } else {
          if (lattice[operands[i]] == "BOTTOM") {

            if (i == 0)
              File << " --> %" << var_names[i + 1] << "=BOTTOM";
            else
              File << ", %" << var_names[i + 1] << "=BOTTOM";
          } else {

            if (i == 0)
              File << " --> %" << var_names[i + 1] << "="
                   << var_values[operands[i]]->getUniqueInteger();
            else
              File << ", %" << var_names[i + 1] << "="
                   << var_values[operands[i]]->getUniqueInteger();
          }
        }
      }
      File << "\n";
    }

    else if (calledFunction->getName() == "__isoc99_scanf") {
      vector<Value *> operands;
      for (unsigned int i = 1; i < callInst->getNumOperands() - 1; i++) {
        operands.push_back(callInst->getOperand(i));
      }
      for (unsigned int i = 0; i < operands.size(); i++) {
        lattice[operands[i]] = "BOTTOM";
        var_values[operands[i]] = NULL;

        if (i == 0)
          File << " --> %" << var_names[i + 1] << "=BOTTOM ";
        else
          File << ", %" << var_names[i + 1] << "=BOTTOM";
      }
      File << "\n";
    }

    else if (calledFunction->getName().startswith(
                 "llvm.nvvm.read.ptx.sreg.ctaid")) {
      if (block_id == -1) {
        var_values[&I] = NULL;
        lattice[&I] = "BOTTOM";
        File << " --> %" << var_names[0] << "=BOTTOM\n";
      } else if (block_id == 0) {
        var_values[&I] = ConstantInt::get(I.getContext(), APInt(32, 0, false));
        lattice[&I] = "CONSTANT";
        File << " --> %" << var_names[0] << "=0\n";
      }

    } else if (calledFunction && calledFunction->getName().startswith(
                                     "llvm.nvvm.read.ptx.sreg.ntid")) {
      if (block_dim == -1) {
        var_values[&I] = NULL;
        lattice[&I] = "BOTTOM";
        File << " --> %" << var_names[0] << "=BOTTOM\n";
      } else if (block_dim == 0) {
        var_values[&I] = ConstantInt::get(I.getContext(), APInt(32, 0, false));
        lattice[&I] = "CONSTANT";
        File << " --> %" << var_names[0] << "=0\n";
      }
    } else if (calledFunction->getName().startswith(
                   "llvm.nvvm.read.ptx.sreg.tid")) {
      if (thread_id == -1) {
        var_values[&I] = NULL;
        lattice[&I] = "BOTTOM";
        File << " --> %" << var_names[0] << "=BOTTOM\n";
      } else if (thread_id == 0) {
        var_values[&I] = ConstantInt::get(I.getContext(), APInt(32, 0, false));
        lattice[&I] = "CONSTANT";
        File << " --> %" << var_names[0] << "=0\n";
      } else if (thread_id == 1) {
        var_values[&I] = ConstantInt::get(I.getContext(), APInt(32, 1, false));
        lattice[&I] = "CONSTANT";
        File << " --> %" << var_names[0] << "=1\n";
      }
    }
  }

  void BranchInst_func(Instruction &I, map<Value *, Constant *> &var_values,
                       map<Value *, string> &lattice,
                       vector<BasicBlock *> preds) {
    File << I;
    BranchInst *branchInst = dyn_cast<BranchInst>(&I);
    if (branchInst->isConditional()) {
      Value *condition = branchInst->getCondition();
      vector<string> var_names = get_var_names(I);
      if (lattice[condition] == "BOTTOM") {
        lattice[condition] = "BOTTOM";
        var_values[condition] = NULL;
        File << " --> %" << var_names[0] << "=BOTTOM\n";
      } else if (lattice[condition] == "CONSTANT") {
        lattice[condition] = "CONSTANT";
        var_values[condition] = var_values[condition];

        File << " --> %" << var_names[0] << "="
             << var_values[condition]->getUniqueInteger() << "\n";
      }
    } else {

      File << "\n";
    }
  }

  void Pointer_instruction(Instruction &I, map<Value *, Constant *> &var_values,
                           map<Value *, string> &lattice,
                           vector<BasicBlock *> preds) {
    File << I;
    GetElementPtrInst *getelementptrInst = dyn_cast<GetElementPtrInst>(&I);
    Value *pointer = getelementptrInst->getPointerOperand();

    lattice[pointer] = "BOTTOM";
    var_values[pointer] = NULL;
    lattice[&I] = "BOTTOM";
    var_values[&I] = NULL;
  }

  // handle sext instruction
  void Sext_Instruction(Instruction &I, map<Value *, Constant *> &var_values,
                        map<Value *, string> &lattice,
                        vector<BasicBlock *> preds) {
    File << I;
    SExtInst *sextInst = dyn_cast<SExtInst>(&I);
    Value *op = sextInst->getOperand(0);
    vector<string> var_names = get_var_names(I);
    if (lattice[op] == "BOTTOM") {
      lattice[&I] = "BOTTOM";
      var_values[&I] = NULL;
      File << " --> %" << var_names[0] << "=BOTTOM\n";
    } else if (lattice[op] == "CONSTANT") {
      lattice[&I] = "CONSTANT";
      var_values[&I] = var_values[op];

      File << " --> %" << var_names[0] << "="
           << var_values[op]->getUniqueInteger() << "\n";
    }
  }

  // zext instruction
  void Zext_Instruction(Instruction &I, map<Value *, Constant *> &var_values,
                        map<Value *, string> &lattice,
                        vector<BasicBlock *> preds) {
    File << I;
    ZExtInst *zextInst = dyn_cast<ZExtInst>(&I);
    Value *op = zextInst->getOperand(0);
    vector<string> var_names = get_var_names(I);
    if (lattice[op] == "BOTTOM") {
      lattice[&I] = "BOTTOM";
      var_values[&I] = NULL;
      File << " --> %" << var_names[0] << "=BOTTOM\n";
    } else if (lattice[op] == "CONSTANT") {
      lattice[&I] = "CONSTANT";
      var_values[&I] = var_values[op];

      File << " --> %" << var_names[0] << "="
           << var_values[op]->getUniqueInteger() << "\n";
    }
  }

  void Switch_Instruction(Instruction &I, map<Value *, Constant *> &var_values,
                          map<Value *, string> &lattice,
                          vector<BasicBlock *> preds) {
    File << I;
    SwitchInst *switchInst = dyn_cast<SwitchInst>(&I);
    Value *condition = switchInst->getCondition();
    vector<string> var_names = get_var_names(I);
    if (lattice[condition] == "BOTTOM") {
      lattice[condition] = "BOTTOM";
      var_values[condition] = NULL;
      File << " --> %" << var_names[0] << "=BOTTOM\n";
    } else if (lattice[condition] == "CONSTANT") {
      lattice[condition] = "CONSTANT";
      var_values[condition] = var_values[condition];

      File << " --> %" << var_names[0] << "="
           << var_values[condition]->getUniqueInteger() << "\n";
    }
  }

  void run_interior_func(Function &F, int &counter, bool &all_block_maps_same,
                         int block_id, int block_dim, int thread_id) {

    all_block_maps_same = false;
    // print function name

    // print function name

    for (BasicBlock &BB : F) {
      map<Value *, Constant *> var_values;
      map<Value *, string> lattice;

      // print block name

      // assign function arguments Values to BOTTOM
      for (auto &arg : F.args()) {
        lattice[&arg] = "BOTTOM";
        var_values[&arg] = NULL;
      }

      vector<BasicBlock *> preds;
      for (BasicBlock *pred : predecessors(&BB)) {
        preds.push_back(pred);
      }

      for (BasicBlock *pred : preds) {
        for (auto it = bb_var_values[pred].begin();
             it != bb_var_values[pred].end(); it++) {
          if (var_values.find(it->first) == var_values.end()) {
            var_values[it->first] = it->second;
            lattice[it->first] = bb_lattice[pred][it->first];
          } else if ((var_values[it->first] != it->second) or
                     (bb_lattice[pred][it->first] != lattice[it->first])) {
            var_values[it->first] = NULL;
            lattice[it->first] = "BOTTOM";
          }
        }
      }

      for (Instruction &I : BB) {
        if (I.getOpcode() == Instruction::Alloca) {
          AllocaInst_func(I, var_values, lattice);
        }

        else if (I.getOpcode() == Instruction::Store) {
          StoreInst_func(I, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::Load) {
          LoadInst_func(I, var_values, lattice, preds);
        }

        else if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
          BinaryOperator_func(I, BO, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::ICmp) {
          ComparisonOperator_func(I, var_values, lattice, preds);
        }

        // else if(I.getOpcode() == Instruction::Ret) {
        //     RetOperator_func(I, var_values, lattice);
        // }

        else if (I.getOpcode() == Instruction::Call) {
          CallInst_func(I, var_values, lattice, preds, block_id, block_dim,
                        thread_id);
        }

        else if (I.getOpcode() == Instruction::Br) {
          BranchInst_func(I, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::GetElementPtr) {
          Pointer_instruction(I, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::Switch) {
          Switch_Instruction(I, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::SExt) {
          Sext_Instruction(I, var_values, lattice, preds);
        }

        else if (I.getOpcode() == Instruction::ZExt) {
          Zext_Instruction(I, var_values, lattice, preds);
        }
      }

      bool same = false;
      // checking if both the lattice and value maps of previous iteration and
      // current iteration are equal or not
      if (bb_var_values[&BB] != var_values and bb_lattice[&BB] != lattice) {
        same = true;
      }
      all_block_maps_same |= same;

      bb_var_values[&BB] = var_values;
      bb_lattice[&BB] = lattice;
    }
    counter++;
  }

  void applyConstantPropagation(Function &F, int block_id, int block_dim,
                                int thread_id) {
    Module *M = F.getParent();
    string input_file_name =
        llvm::sys::path::filename(M->getModuleIdentifier()).str();
    Input_File = input_file_name;

    int counter = 0;
    bool all_block_maps_same = false;
    do {
      run_interior_func(F, counter, all_block_maps_same, block_id, block_dim,
                        thread_id);
    } while ((all_block_maps_same && counter < 100));

    counter--;
    llvm::raw_fd_ostream File(
        "temp.txt", EC,
        llvm::sys::fs::OF_None); // clearing the previous content of the file
    run_interior_func(
        F, counter, all_block_maps_same, block_id, block_dim,
        thread_id); // printing the final iteration of the unchanged values
    remove_first_line_of_file(); // removing the first line of the file (some
                                 // random thing is getting printed)
  }

  /* Cuda Bug Detection Functions */
  vector<string> get_pointerArguments(Function &F) {
    vector<string> args;
    unsigned idx = 0;
    for (auto &arg : F.args()) {
      if (arg.getType()->isPointerTy()) {
        string argName = arg.getName().str();
        if (argName.empty()) {
          argName = to_string(idx);
        }
        args.push_back(argName);
      }
      idx++;
    }
    return args;
  }

  void set_Block_and_Predecessor_names(Function &F) {
    int bbIndex = 0;

    // First pass: assign names to unnamed blocks
    for (BasicBlock &BB : F) {
      if (!BB.hasName()) {
        // BB.setName("bb" + std::to_string(bbIndex++));
        block_name[&BB] = "bb" + std::to_string(bbIndex++);
      }
    }
  }

  void getRegisterName(Instruction &I) {
    // for load instruction
    string inst;
    raw_string_ostream rso(inst);
    I.print(rso);
  }

  vector<string> get_operands(Instruction &I) {
    vector<string> operands;
    string inst;
    raw_string_ostream rso(inst);
    I.print(rso);
    string temp;
    for (long unsigned int i = 0; i < inst.size(); i++) {
      if (inst[i] == '%') {
        temp = "";
        i++;
        while (i != inst.size() and (isalpha(inst[i]) or isdigit(inst[i]))) {
          temp += inst[i];
          i++;
        }
        operands.push_back(temp);
      }
    }
    return operands;
  }

  void buildTidDependencyMap(Instruction &I) {
    // load instruction
    if (auto *loadInst = dyn_cast<LoadInst>(&I)) {
      vector<string> operands = get_operands(I);
      var_dep_on_tid[operands[0]].push_back(operands[1]);
    }
    // store instruction
    else if (auto *storeInst = dyn_cast<StoreInst>(&I)) {
      vector<string> operands = get_operands(I);
      if (operands.size() > 1) {
        var_dep_on_tid[operands[1]].push_back(operands[0]);
      } else
        var_dep_on_tid[operands[0]].push_back(operands[0]);
    }
    // binary operator
    else if (auto *binaryOp = dyn_cast<BinaryOperator>(&I)) {
      vector<string> operands = get_operands(I);
      for (int i = 1; i < operands.size(); i++)
        var_dep_on_tid[operands[0]].push_back(operands[i]);
    }

    // sext instruction
    else if (auto *sextInst = dyn_cast<SExtInst>(&I)) {
      vector<string> operands = get_operands(I);
      var_dep_on_tid[operands[0]].push_back(operands[1]);
    }

    // sitof instruction
    else if (auto *sitofInst = dyn_cast<SIToFPInst>(&I)) {
      vector<string> operands = get_operands(I);
      var_dep_on_tid[operands[0]].push_back(operands[1]);
    }

    // getelementptr instruction
    else if (auto *getElementPtrInst = dyn_cast<GetElementPtrInst>(&I)) {
      vector<string> operands = get_operands(I);
      var_dep_on_tid[operands[0]].push_back(operands[1]);
      var_dep_on_tid[operands[0]].push_back(operands[2]);
    }

    // compare instruction
    else if (auto *compareInst = dyn_cast<ICmpInst>(&I)) {
      vector<string> operands = get_operands(I);
      for (int i = 1; i < operands.size(); i++)
        var_dep_on_tid[operands[0]].push_back(operands[i]);
    }

    else if (auto *callInst = dyn_cast<CallInst>(&I)) {
      Function *calledFunction = callInst->getCalledFunction();
      // if (calledFunction && calledFunction->getName() ==
      // "llvm.nvvm.barrier0") {
      //     // Handle barrier function call
      // }
      if (calledFunction &&
          calledFunction->getName().startswith("llvm.nvvm.read.ptx.sreg.tid")) {
        vector<string> operands = get_operands(I);
        var_dep_on_tid[operands[0]].push_back(calledFunction->getName().str());
      }
    }

    // handle fcmp instruction
    else if (auto *fcmpInst = dyn_cast<FCmpInst>(&I)) {
      vector<string> operands = get_operands(I);
      for (int i = 1; i < operands.size(); i++)
        var_dep_on_tid[operands[0]].push_back(operands[i]);
    }
  }

  bool check_InstDependencyOnTid(Instruction &I) {
    vector<string> operands = get_operands(I);
    if (operands.size() < 2)
      return false;

    set<string> visited;
    stack<string> dfs_stack;
    string check_variable;
    // if the instruction is not store, then check_variable is the first operand
    // else for the store instruction, check_variable is the second operand
    if (I.getOpcode() == Instruction::Store) {
      check_variable = operands[1];
    } else {
      check_variable = operands[0];
    }
    dfs_stack.push(check_variable);

    while (!dfs_stack.empty()) {
      string curr = dfs_stack.top();
      dfs_stack.pop();

      if (curr.empty() || visited.count(curr))
        continue;
      visited.insert(curr);

      // Base case: direct thread ID access
      if (curr == "llvm.nvvm.read.ptx.sreg.tid.x" ||
          curr == "llvm.nvvm.read.ptx.sreg.tid.y" ||
          curr == "llvm.nvvm.read.ptx.sreg.tid.z") {
        return true;
      }

      // Recurse on dependencies
      if (var_dep_on_tid.find(curr) != var_dep_on_tid.end()) {
        for (const string &dep : var_dep_on_tid[curr]) {
          if (!dep.empty())
            dfs_stack.push(dep);
        }
      }
    }

    return false;
  }

  bool check_VarDependencyOnTid(string s) {
    if (s.empty())
      return false;

    set<string> visited;
    stack<string> dfs_stack;
    dfs_stack.push(s);

    while (!dfs_stack.empty()) {
      string curr = dfs_stack.top();
      dfs_stack.pop();

      if (curr.empty() || visited.count(curr))
        continue;
      visited.insert(curr);

      // Base case: direct thread ID access
      if (curr == "llvm.nvvm.read.ptx.sreg.tid.x" ||
          curr == "llvm.nvvm.read.ptx.sreg.tid.y" ||
          curr == "llvm.nvvm.read.ptx.sreg.tid.z") {
        return true;
      }

      // Recurse on dependencies
      if (var_dep_on_tid.find(curr) != var_dep_on_tid.end()) {
        for (const string &dep : var_dep_on_tid[curr]) {
          if (!dep.empty())
            dfs_stack.push(dep);
        }
      }
    }

    return false;
  }

  bool checkDependencyOnSharedMem(Instruction &I,
                                  vector<string> &args_of_func) {
    vector<string> operands = get_operands(I);
    if (operands.empty())
      return false;

    set<string> visited;
    stack<string> dfs_stack;

    for (const string &op : operands) {
      if (!op.empty())
        dfs_stack.push(op);
    }

    while (!dfs_stack.empty()) {
      string curr = dfs_stack.top();
      dfs_stack.pop();

      if (curr.empty() || visited.count(curr))
        continue;
      visited.insert(curr);

      // Check if current operand is a function argument (e.g., "0", "1", ...)
      if (std::find(args_of_func.begin(), args_of_func.end(), curr) !=
          args_of_func.end()) {
        return true;
      }

      // Recurse through variable dependencies
      if (var_dep_on_tid.find(curr) != var_dep_on_tid.end()) {
        for (const string &dep : var_dep_on_tid[curr]) {
          if (!dep.empty())
            dfs_stack.push(dep);
        }
      }
    }

    return false;
  }

  string getSymbol_of_BInaryOPerator(Instruction &I) {
    string symbol;
    if (I.getOpcode() == Instruction::Add) {
      symbol = "+";
    } else if (I.getOpcode() == Instruction::Sub) {
      symbol = "-";
    } else if (I.getOpcode() == Instruction::Mul) {
      symbol = "*";
    } else if (I.getOpcode() == Instruction::SDiv or
               I.getOpcode() == Instruction::UDiv or
               I.getOpcode() == Instruction::FDiv) {
      symbol = "/";
    }
    // } else if (I.getOpcode() == Instruction::URem) {
    //     symbol = "%";
    // } else if (I.getOpcode() == Instruction::And) {
    //     symbol = "&";
    // } else if (I.getOpcode() == Instruction::Or) {
    //     symbol = "|";
    // } else if (I.getOpcode() == Instruction::Xor) {
    //     symbol = "^";
    // }
    return symbol;
  }

  string getSymbol_of_ComparisonOperator(Instruction &I) {
    if (ICmpInst *icmp = dyn_cast<ICmpInst>(&I)) {
      switch (icmp->getPredicate()) {
      case ICmpInst::ICMP_EQ:
        return "==";
      case ICmpInst::ICMP_NE:
        return "!=";
      case ICmpInst::ICMP_SLT:
        return "<";
      case ICmpInst::ICMP_SGT:
        return ">";
      case ICmpInst::ICMP_SLE:
        return "<=";
      case ICmpInst::ICMP_SGE:
        return ">=";
      case ICmpInst::ICMP_ULT:
        return "< (unsigned)";
      case ICmpInst::ICMP_UGT:
        return "> (unsigned)";
      case ICmpInst::ICMP_ULE:
        return "<= (unsigned)";
      case ICmpInst::ICMP_UGE:
        return ">= (unsigned)";
      default:
        return "unknown_predicate";
      }
    }
    return "not_icmp";
  }

  string ConvertValToString(Value *val) {
    string str;
    raw_string_ostream rso(str);
    val->print(rso);

    string ans = rso.str();
    // traverse from the end of string until you find space
    for (int i = ans.size() - 1; i >= 0; i--) {
      if (ans[i] == ' ') {
        ans = ans.substr(i + 1, ans.size() - i - 1);
        break;
      }
    }
    return ans;
  }

  void get_a_and_b_values(Function &F, int block_id, int block_dim,
                          int thread_id) {
    applyConstantPropagation(F, block_id, block_dim, thread_id);
  }

  // redundancy checking

  using AccessMap = map<string, pair<pair<string, string>, string>>;
  vector<AccessMap> regions;

  map<BasicBlock *, map<Value *, Constant *>> bb_var_values0;
  map<BasicBlock *, map<Value *, Constant *>> bb_var_values1;
  map<BasicBlock *, map<Value *, string>> bb_lattice0;
  map<BasicBlock *, map<Value *, string>> bb_lattice1;
  string getName(Value *V) {
    string s;
    raw_string_ostream rso(s);
    V->printAsOperand(rso, false);
    return rso.str();
  }

  // void detectConflicts() {

  //   for (size_t i = 1; i < regions.size(); ++i) {
  //     const auto &prev = regions[i - 1];
  //     const auto &curr = regions[i];

  //     // errs() << "\n--- Comparing Region #" << i - 1 << " and #" << i << "
  //     // ---\n";
  //     bool redundant = true;

  //     for (const auto &[var, prevAccesses] : prev) {
  //       auto it = curr.find(var);
  //       if (it == curr.end())
  //         continue;

  //       const auto &currAccesses = it->second;
  //       // get last access from prevaccesses and first access from curraccesses

  //       auto lastAccess = prevAccesses.back().first;
  //       auto firstAccess = currAccesses.front().first;
  //       auto lastType = prevAccesses.back().second;
  //       auto firstType = currAccesses.front().second;
  //       auto lastA = lastAccess.first;
  //       auto lastB = lastAccess.second;
  //       auto firstA = firstAccess.first;
  //       auto firstB = firstAccess.second;

  //       // check if the last access and first access are same as access are
  //       // linear eq of tid where access is a*tid+b if both access are same then
  //       // both linear eq are same lastA==firstA && lastB==firstB

  //       bool accessequal = lastA == firstA && lastB == firstB;
  //       bool typeequal = lastType == "read" && firstType == "read";

  //       if (lastA == "BOTTOM" || lastB == "BOTTOM" || firstA == "BOTTOM" ||
  //           firstB == "BOTTOM") {
  //         accessequal = true;
  //       }
  //       if (lastA == "0" && firstA == "0") {
  //         accessequal = true;
  //       }

  //       if (!accessequal && !typeequal) {
  //         redundant &= false;
  //       } else
  //         redundant &= true;
  //     }
  //     if (redundant) {
  //       // outfile << "Redundant barrier detected between regions #" << i - 1
  //       //         << " and #" << i << "\n";
  //       outfile << "barrier " << i << " is redundant\n";
  //     } else {
  //       // outfile << "No redundancy detected between regions #" << i - 1
  //       //         << " and #" << i << "\n";
  //       outfile << "barrier " << i << " is not redundant\n";
  //     }
  //     outfile << "-----------------------------------\n";
  //   }
  // }

  bool dataracecheck(Function &F) {
    errs() << F.getName() << "\n";
    // errs()<<regions.size()<< "-----------------------------------\n";
    // regions.clear();
    AccessMap current;

    for (auto &BB : F) {
      for (auto &I : BB) {
        errs() << I << "\n";
        if (auto *call = dyn_cast<CallInst>(&I)) {
          errs() << regions.size() << "-----------------------------------\n";
          Function *calledFunc = call->getCalledFunction();
          if (calledFunc &&
              calledFunc->getName().contains("llvm.nvvm.barrier0")) {
            // regions.push_back(current);
            current.clear();
            continue;
          }
        }

        Value *ptr = nullptr;
        string type;
        string baseName, indexStr = "unknown";

        if (auto *load = dyn_cast<LoadInst>(&I)) {

          ptr = load->getPointerOperand();
          if (ptr) {
            if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {

              Value *base = gep->getPointerOperand();
              // base is always load instruction get the pointer operand into
              // baseName
              if (LoadInst *load = dyn_cast<LoadInst>(base)) {
                Value *loadPtr =
                    load->getPointerOperand(); // The operand being loaded from
                baseName = getName(loadPtr);
              }

              string a, b;
              Value *index = gep->getOperand(1);
              if (ConstantInt *CI = dyn_cast<ConstantInt>(index)) {
                a = "0"; // Coefficient of tid is 0 since it's a constant index
                b = std::to_string(CI->getZExtValue()); // Offset
              }

              else {
                if (bb_lattice0[&BB][index] == "CONSTANT") {
                  b = std::to_string(bb_var_values0[&BB][index]
                                         ->getUniqueInteger()
                                         .getSExtValue());
                } else if (bb_lattice0[&BB][index] == "BOTTOM") {
                  b = "BOTTOM";
                }

                if (bb_lattice1[&BB][index] == "CONSTANT") {
                  a = std::to_string(bb_var_values1[&BB][index]
                                         ->getUniqueInteger()
                                         .getSExtValue());
                } else if (bb_lattice1[&BB][index] == "BOTTOM") {
                  a = "BOTTOM";
                }
                if (a == "BOTTOM" || b == "BOTTOM") {
                  a = "BOTTOM";
                  b = "BOTTOM";
                } else {
                  int temp = stoi(a) - stoi(b);
                  a = std::to_string(temp);
                }
              }

              indexStr = getName(index);
              type = "read";
              // get  value if current[baseName] is not empty get value from it

              if (current.count(baseName) == 0) {
                current[baseName] = {{a, b}, type};
                errs() << "Read from " << baseName << "[" << a << "]\n";
                errs() << "Read from " << baseName << "[" << b << "]\n";
              } else {
                auto lastAccess = current[baseName].first;
                auto lastType = current[baseName].second;
                if (lastType != "read") {
                  string lastA = lastAccess.first;
                  string lastB = lastAccess.second;
                  if (lastA == "BOTTOM" || lastB == "BOTTOM" || a == "BOTTOM" ||
                      b == "BOTTOM") {
                    outfile << "datarace at " << baseName << "\n";
                  } else if (a == "0" && b == "0") {
                    outfile << "no datarace at " << baseName << "\n";
                  } else if (lastA != a || lastB != b) {
                    outfile << "datarace at " << baseName << "\n";
                  } else {
                    outfile << "no datarace at " << baseName << "\n";
                  }
                }
                current[baseName] = {{a, b}, type};
              }
            }
          }
        } else if (auto *store = dyn_cast<StoreInst>(&I)) {
          ptr = store->getPointerOperand();
          if (ptr) {
            if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {

              Value *base = gep->getPointerOperand();
              // base is always load instruction get the pointer operand into
              // baseName
              if (LoadInst *load = dyn_cast<LoadInst>(base)) {
                Value *loadPtr =
                    load->getPointerOperand(); // The operand being loaded from
                baseName = getName(loadPtr);
              }
              // check if gep->getPointerOperand() is a constant int or not

              std::string a, b;
              Value *index = gep->getOperand(1);
              if (ConstantInt *CI = dyn_cast<ConstantInt>(index)) {
                a = "0"; // Coefficient of tid is 0 since it's a constant index
                b = std::to_string(CI->getZExtValue()); // Offset
              }

              else {
                if (bb_lattice0[&BB][index] == "CONSTANT") {
                  b = std::to_string(bb_var_values0[&BB][index]
                                         ->getUniqueInteger()
                                         .getSExtValue());
                } else if (bb_lattice0[&BB][index] == "BOTTOM") {
                  b = "BOTTOM";
                }

                if (bb_lattice1[&BB][index] == "CONSTANT") {
                  a = std::to_string(bb_var_values1[&BB][index]
                                         ->getUniqueInteger()
                                         .getSExtValue());
                } else if (bb_lattice1[&BB][index] == "BOTTOM") {
                  a = "BOTTOM";
                }
                if (a == "BOTTOM" || b == "BOTTOM") {
                  a = "BOTTOM";
                  b = "BOTTOM";
                } else {
                  int temp = stoi(a) - stoi(b);
                  a = std::to_string(temp);
                }
              }

              indexStr = getName(index);
              type = "write";
              if (current.count(baseName) == 0) {
                current[baseName] = {{a, b}, type};
                errs() << "Read from " << baseName << "[" << a << "]\n";
                errs() << "Read from " << baseName << "[" << b << "]\n";
              } else {
                auto lastAccess = current[baseName].first;
                auto lastType = current[baseName].second;
                string lastA = lastAccess.first;
                string lastB = lastAccess.second;
                if (lastA == "BOTTOM" || lastB == "BOTTOM" || a == "BOTTOM" ||
                    b == "BOTTOM") {
                  outfile << "datarace at " << baseName << "\n";
                } else if (a == "0" && b == "0") {
                  outfile << "no datarace at " << baseName << "\n";
                } else if (lastA != a || lastB != b) {
                  outfile << "datarace at " << baseName << "\n";
                } else {
                  outfile << "no datarace at " << baseName << "\n";
                }

                current[baseName] = {{a, b}, type};
              }
            }
          }
        }
      }
    }

    // if (!current.empty())
    regions.push_back(current);

    outfile << "-----------------------------------\n";
    outfile << F.getName() << "\n";
    outfile << "-----------------------------------\n";
    // detectConflicts();
    return false;
  }

  bool runOnFunction(Function &F) override {
    get_a_and_b_values(F, 0, 0, 0);
    bb_var_values0 = bb_var_values;
    bb_lattice0 = bb_lattice;
    // clear bb_var_values and bb_lattice
    bb_var_values.clear();
    bb_lattice.clear();

    get_a_and_b_values(F, 0, 0, 1);
    bb_var_values1 = bb_var_values;
    bb_lattice1 = bb_lattice;

    // call redundancy check function to check barrier redundancy
    dataracecheck(F);
    return false;
  }
};
} // namespace

char Hello::ID = 0;
static RegisterPass<Hello> X(
    "Hello",
    "Detects and Reports Shared Memory Accesses and Potential Barrier Issues");
