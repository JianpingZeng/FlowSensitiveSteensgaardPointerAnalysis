//#include <bdd.h>
//#include "fdd.h"
#include "points_to_graph.cpp"

using namespace llvm;

class Solver{
  //bdd x;
  private:
  Points_to_Graph graph;
  int lineno = 0;
  int lineptr = 0; //stores the line number of a PtrToInt instruction
  Variable ptr; //variable used to store the operand of a PtrToInt instruction
  
  //below the methods for handle each LLVM's IR instruction are implemented
  void handleAlloca(Instruction* I){
    Node n;
    Variable v = I->getName();
    n.points_to_variables.push_back(v);
    n.next=nullptr;
    graph.insertNode(n,v);
  }
    
  void handleStore(Instruction* I){
    Variable v1 = ((lineno - lineptr == 1) ? ptr : I->getOperand(0)->getName());
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
    
  void handlePtrToInt(Instruction* I){
    ptr = I->getOperand(0)->getName();
    lineptr = lineno;
  }
    
  public:
  bool runOnModule(Module &M){
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
	  }
	}
      }
    }
    //bdd_done();
    graph.print();
    return false;
    }
};