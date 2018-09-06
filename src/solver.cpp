//#include <bdd.h>
//#include "fdd.h"
#include "../include/solver.h"
#include "points_to_graph.cpp"

//Below the methods for handle each LLVM's IR instruction are implemented
void Solver::handleAlloca(Instruction* I){
  Node n;
  Variable v = I->getName();
  n.points_to_variables.push_back(v);
  n.next=nullptr;
  graph.insertNode(n,v);
}

void Solver::handleStore(Instruction* I){
  Variable v1 = ((lineno - lineptr == 1) ? ptrOperand : I->getOperand(0)->getName());
  v1 = ((copyConstraint) ? loadOperand : v1);
  Variable v2 = I->getOperand(1)->getName();
  Node *node1 = graph.findNode(v2);
  Node *node2 = graph.findNode(v1);
  if(node1!=nullptr && node2!=nullptr){
    if(node1->next == nullptr){
      node1->next = node2;
    }
    else{
      //In case of a copy constraint, we call the appropriate method
      (copyConstraint) ? handleStoreInCopyConstraint(node1, node2) : graph.merge(node1->next,node2);
      //We set the flag to false since we already treated the contraint at this point
      copyConstraint = false;
    }
  }
}

void Solver::handleStoreInCopyConstraint(Node* nodeLeft, Node* nodeRight){
  //We create this method because the treatment of a copy instruction is a little bit different.
  //In a copy instruction we must merge the points-to sets of both nodes; and the both origin nodes must point to this new node, what doesn't 
  //happen when we simple merge, since we delete the points-to set of second node to save memory. Therefore, we
  //save the reference to the second node and after the merge we make the node point to the new node created. 
  Node *refNodeRight = nodeRight;
  graph.merge(nodeLeft->next,nodeRight->next);
  refNodeRight->next = nodeLeft->next;
}
  
void Solver::handlePtrToInt(Instruction* I){
  ptrOperand = I->getOperand(0)->getName();
  lineptr = lineno;
}
  
void Solver::handleLoad(Instruction* I){
  loadOperand = I->getOperand(0)->getName();
  //Steensgaard Analysis ignore the statement if the points-to set of the right-side operator is empty
  //In this case, merge it's not necessary
  if(graph.findNode(loadOperand)->next==nullptr) 
    return;
  copyConstraint = true;
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