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
  vector <Node> nodes;
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
    for (unsigned i=0; i<graph.nodes.size(); i++){
      //((graph.nodes[i].points_to_variable[0] == node_name) ? return graph.nodes[i]: return nullptr);
      if(graph.nodes[i].points_to_variables[0] == node_name){
	//llvm::errs() << "deu certo para: "<< node_name <<"!!!!!\n";
	return &graph.nodes[i];
      }
    }
    return nullptr;
  }
  
  void printGraph(){
    Node *node = nullptr;
    cout << "\ngrafo final " <<"\n";
    for (unsigned i=0;i<this->graph.nodes.size();i++){
      node = &graph.nodes[i];
      while(node!=nullptr){
	llvm::errs() << node->points_to_variables[0] << " -> ";
	node=node->next;
      }
      errs() <<"\n";
    }
      //llvm::errs() << "nos: " << graph.nodes[i].points_to_variables[0];
      //for (unsigned j =0;j<graph.nodes[i].points_to_variables.size();j++)
	//llvm::errs() << graph.nodes[i].points_to_variables[j] << " ";
  }
  
  bool runOnModule(Module &M){
    llvm::errs() << "Nome do modulo: " << M.getName() <<"\n";
    for(Function &F: M){
      llvm::errs() << "Nome da funcao: " << F.getName() <<"\n";
      for (BasicBlock &bb: F) {
	for (Instruction &I: bb) {
	  lineno++;
	  switch(I.getOpcode()){
	    case Instruction::Alloca: {
	      Node n;
	      Variable v = I.getName();
	      n.points_to_variables.push_back(v);
	      n.next=nullptr;
	      graph.nodes.push_back(n);
	      break;
	    }
	    case Instruction::Store:{
	      Variable v1 = ((lineno - lineptr == 1) ? ptr : I.getOperand(0)->getName());
	      Variable v2 = I.getOperand(1)->getName();
	      //llvm::errs() << "comando " <<v2 <<" = " <<v1 <<"\n";
	      Node *node1 = findNode(v2);
	      Node *node2 = findNode(v1); //CONSERTAR ISSO DAQUI, ACHO INEFICIENTE TER QUE FAZER DUAS PESQUISAS!!!!!!
	      if(node1!=nullptr && node2!=nullptr){
		if(node1->next == nullptr){
		  node1->next = node2;;
		  //llvm::errs() << "no " << node1->points_to_variables[0] <<" aponta para "<< node1->next->points_to_variables[0]<<"\n";
		}
	      }
	      //Node *j = findNode(v1);
	      //merge(findNode(v2),findNode(v1));
	      //if(j!=nullptr){
		//llvm::errs() << "encontrado o no: " <<j->points_to_variables[0] <<"\n";
		
	      //}
	      //merge(findNode(v2),findNode(v1));
		//llvm::errs() << v1 <<" " << v2 << "\n\n";
	      
	      
	      break;
	    }
	    case Instruction::PtrToInt:{
	      //llvm::errs() << "olha a instrucao " <<I.getOperand(0)->getName() <<"\n";
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