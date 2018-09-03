#include <iostream>
//#include <bdd.h>
//#include "fdd.h"
#include <vector>

using namespace std;
using namespace llvm;

typedef StringRef Variable;


struct Node{
  vector <Variable> points_to_variables;
  Node *next;
};

struct Points_to_graph{
  map<Variable, Node> nodes;
};

class Solver{
  //bdd x;
  Points_to_graph graph;
  int lineno = 0;
  int lineptr = 0; //stores the line number of a PtrToInt instruction
  Variable ptr; //variable used to store the operand of a PtrToInt instruction
  public:
    
  void merge(Node *n1, Node *n2){
    if(n1!=nullptr){
      n1->points_to_variables.insert(n1->points_to_variables.end(), n2->points_to_variables.begin(), n2->points_to_variables.end());
      for (unsigned i=0; i<n1->points_to_variables.size(); i++)
	llvm::errs() << n1->points_to_variables[i] <<" ";
	
      llvm::errs() <<"apos o merge \n";
    }
  }
    
  Node* findNode(Variable node_name){
    // here we simple check if a node exists in the graph
    // in positive case we return a reference to it, otherwise we return a null pointer
    return ((graph.nodes.count(node_name) == 1) ? &graph.nodes[node_name] : nullptr);
  }
  
  void printGraph(){
    Node *node = nullptr;
    cout << "\ngrafo final " <<"\n";
    for(auto &i: graph.nodes){
      node = &i.second;
      while(node!=nullptr){
	for(int j=0; j<node->points_to_variables.size(); ++j) //print all variables in a set of a node
	  llvm::errs() << node->points_to_variables[j]; 
	llvm::errs() << " -> ";
	node=node->next;
      }
      errs() <<"\n";
    }
  }
  
  bool runOnModule(Module &M){
    llvm::errs() << "Nome do modulo: " << M.getName() <<"\n";
    for(Function &F: M){
      llvm::errs() << "Nome da funcao: " << F.getName() <<"\n";
      for (BasicBlock &bb: F) {
	for (Instruction &I: bb) {
	  lineno++;
	  switch(I.getOpcode()){
	    case Instruction::Alloca:{
	      Node n;
	      Variable v = I.getName();
	      n.points_to_variables.push_back(v);
	      n.next=nullptr;
	      graph.nodes[v]=n;
	      break;
	    }
	    case Instruction::Store:{
	      Variable v1 = ((lineno - lineptr == 1) ? ptr : I.getOperand(0)->getName());
	      Variable v2 = I.getOperand(1)->getName();
	      Node *node1 = findNode(v2);
	      Node *node2 = findNode(v1);
	      if(node1!=nullptr && node2!=nullptr){
		if(node1->next == nullptr){
		  node1->next = node2;
		}
		//else
		//merge(node1,node2)
	      }
	      break;
	    }
	    case Instruction::PtrToInt:{
	      ptr = I.getOperand(0)->getName();
	      lineptr = lineno;
	    }
	      
	  }
	}
      }
    }
    //bdd_done();
    printGraph();
    return false;
    }
};