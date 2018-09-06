//#include <bdd.h>
//#include "fdd.h"
#include "../include/solver.h"
#include "points_to_graph.cpp"

//below the methods for handle each LLVM's IR instruction are implemented
void Solver::handleAlloca(Instruction* I){
  Node n;
  Variable v = I->getName();
  n.points_to_variables.push_back(v);
  n.next=nullptr;
  graph.insertNode(n,v);
}
  
void Solver::handleStore(Instruction* I){
  Variable v1 = ((lineno - lineptr == 1) ? ptrOperand : I->getOperand(0)->getName());
  v1 = ((lineno - lineload == 1) ? loadOperand : v1);
  Variable v2 = I->getOperand(1)->getName();
  Node *node1 = graph.findNode(v2);
  Node *node2 = graph.findNode(v1);
  if(node1!=nullptr && node2!=nullptr){
    if(node1->next == nullptr){
      node1->next = node2;
    }
    else{
      graph.merge(node1->next,node2);
    }
  }
}
  
void Solver::handlePtrToInt(Instruction* I){
  ptrOperand = I->getOperand(0)->getName();
  lineptr = lineno;
}
  
void Solver::handleLoad(Instruction* I){
  loadOperand = I->getOperand(0)->getName();
  lineload = lineno;
}
  
  
bool Solver::runOnModule(Module &M){
  errs() << "Nome do modulo: " << M.getName() <<"\n";
  for(Function &F: M){
    errs() << "Nome da funcao: " << F.getName() <<"\n";
    for (BasicBlock &bb: F) {
      for (Instruction &I: bb) {
	lineno++;
	switch(I.getOpcode()){
	  case Instruction::Alloca:
	    handleAlloca(&I);
	    break;
	  case Instruction::Store:
	    handleStore(&I);
	    break;
	  case Instruction::PtrToInt:
	    handlePtrToInt(&I);
	    break;
	  case Instruction::Load:
	    handleLoad(&I);
	    break;
	}
      }
    }
  }
  //bdd_done();
  graph.print();
  return false;
}