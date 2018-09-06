#define DEBUG_TYPE "FlowSensitiveSteensgaard"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Twine.h" //SE RESOLVER INSERIR AS VARIAVEIS SEM O ADDR, PODE RETIRAR ESSE INCLUDE
#include "./src/solver.cpp" //external code

using namespace llvm;

namespace{
    struct FlowSensitiveSteensgaard: public ModulePass{
      static char ID;
      Solver solver;
      FlowSensitiveSteensgaard(): ModulePass(ID){}
      bool runOnModule(Module &M){
	solver.runOnModule(M);
	return false;
      }
    };
  
}
char FlowSensitiveSteensgaard::ID = 0;
static RegisterPass<FlowSensitiveSteensgaard> X("FlowSensitiveSteensgaard", "Flow-Sensitive Steensgaard Pointer Analysis");