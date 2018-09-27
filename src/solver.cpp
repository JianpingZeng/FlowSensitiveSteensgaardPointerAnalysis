#include "../include/solver.h"
#include "points_to_graph.cpp"
#include <sstream>

//Below the methods for handle each LLVM's IR instruction are implemented
void Solver::handleAlloca(Variable name_node){
  Node n;
  n.name = name_node;
  n.points_to_variables.push_back(name_node);
  n.next=nullptr;
  graph.insertNode(n,name_node);
}

void Solver::handleStore(Instruction* I){
  Variable v1 = ((lineno - lineptr == 1) ? ptrOperand : I->getOperand(0)->getName());
  v1 = ((copyConstraint) ? loadVariables.back() : v1);
  Variable v2 = I->getOperand(1)->getName();
  Node *node1 = graph.findNode(v2);
  Node *node2 = graph.findNode(v1);
  if(node1!=nullptr && node2!=nullptr){
    if(falseBlock){
      handleStoreInFalseBlock(I->getFunction(), node1, node2);
      copyConstraint = false;
      return;
    }
    if(delTemporaryNode){ //If we are storing a value from a temporary node, we must delete this node, because it's not exists in the source code
       //errs() <<"no 1: " <<node1->name << " no 2: " <<node2->next->name <<"\n";
      node1->next = node2->next;
      graph.deleteNode(node2->name);
      delTemporaryNode = false;
      return;
    }
    //In case of a copy constraint, we call the appropriate method
    (copyConstraint) ? node1->next = node2->next : node1->next=node2;      
    //We set the flag to false since we already treated the contraint at this point
    copyConstraint = false;
  }
  
}

void Solver::handleStoreInFalseBlock(Function*F, Node* node1, Node* node2){
  if(node1->next!=nullptr){
    /*Node n = *node1->next;
    node1->next->points_to_variables.push_back(node2->name);
    Twine name_node_twine(n.name, "#");
    Variable node_name(name_node_twine.str());
    n.name=node_name;
    graph.insertNode(n, node_name);*/
    Node n = *node1->next;
    n.points_to_variables.push_back(node2->name);
    string s2 = to_string(ifCounter);
    string s3 = F->getName().str()+"if"+to_string(ifCounter);
    Variable node_name = s3;
    n.name=node_name;
    graph.insertNode(n, node_name);
    node1->next = graph.findNode(node_name);
  }
}

void Solver::handleStoreInCopyConstraint(Node* nodeLeft, Node* nodeRight){
  //We create this method because the treatment of a copy instruction is a little bit different.
  //In a copy instruction we must merge the points-to sets of both nodes; and the both origin nodes must point to this new node, what doesn't 
  //happen when we simple merge, since we delete the points-to set of second node to save memory. Therefore, we
  //save the reference to the second node and after the merge we make the node point to the new node created.
  Node *refNodeRight = nodeRight;
  nodeLeft->next = nodeRight->next;
  //graph.merge(nodeLeft->next,nodeRight->next);
  //refNodeRight->next = nodeLeft->next;
}
  
void Solver::handlePtrToInt(Instruction* I){
  ptrOperand = I->getOperand(0)->getName();
  lineptr = lineno;
}

void Solver::handleIntToPtr(){
   lineptr = lineno;
   copyConstraint = false;
}

void Solver::handleLoad(Instruction* I){
  loadOperand = I->getOperand(0)->getName();
  if(!loadOperand.empty()){ //If the operand is not a IR's temporary
    loadVariables.push_back(loadOperand);
    //Steensgaard Analysis ignore the statement if the points-to set of the right-side operator is empty
    //In this case, merge it's not necessary
  if(graph.findNode(loadOperand)->next==nullptr) 
      return;
    copyConstraint = true;
  }
} 
  
void Solver::handleCall(Instruction* I){
  Function *F = cast<CallInst>(*I).getCalledFunction(); //We cast the instruction to a CallInst to get information about the CALLEE?????? function
  //When occurs a function call that have some kind of return, LLVM's IR store this value in a temporary called %call
  //LLVM's Instruction inherits from Value class, so we can use it's name as a Value.
  if(!I->getName().empty()){ //If the name of the Instruction is not empty, we have a %call temporary receiving the function call
    //A new node is created to this temporary because we need merge it with the variable which receives the function result in the source code
    Node n;
    Variable v = I->getName();
    n.name = v;
    n.points_to_variables.push_back(v);
    Twine node_next(F->getName(), "%");
    Variable node_next_name(node_next.str());
    n.next=graph.findNode(node_next_name)->next; //It's points-to set is the points-to set of the return node of the function we are handling
    graph.insertNode(n,v);
    copyConstraint = false;
    delTemporaryNode = true;
  }
  unsigned i = F->arg_size();
  unsigned k = loadVariables.size();
  for(auto arg = F->arg_begin(); arg != F->arg_end(); ++arg) { //Here we get the formal parameters of the function
    Twine argName(arg->getName(), ".addr");
    Variable node_name(argName.str());
    //The LLVM loads all the real parameters in temporaries before the function call. Those parameters are stored in the loadVariables vector
    //Due the C's passing parameters mechanism, we merge the parameters treating the function call like a copy constraint
    handleStoreInCopyConstraint(graph.findNode(loadVariables[k-i]),graph.findNode(node_name));
    i--;
  }
  copyConstraint = false;
}

void Solver::handleRet(Instruction* I){
  //A Ret instruction is equivalet a return of a function in the source code. Independetly of how many return statements the function has,
  //the LLVM's IR create a single basic block with a Ret instruction. A Load instruction occurs to get the return value from a temporary, 
  //however this doesn't characterize a Copy Constraint, thus we set the flag to false. 
   copyConstraint = false;
   ReturnInst *ri = dyn_cast<ReturnInst>(I);
   if(ri->getNumOperands()!=0){
     //The LLVM's IR create a single temporary to the return value called retval. So we have to change it1s node because we'll work with many functions
     //We simply "change the key" of the node to the name of the function that returns. Of course, we do this if the function has some return value,
     //that is, a non-void return
     Twine newFunctionReturnName(I->getFunction()->getName(), "%");
     //We add the "%" symbol in the name of the node to garantee that won't be a identifier name equal to the function. Identifiers's name in C
     //can't contain special characters
     Variable node_name(newFunctionReturnName.str());
     graph.updateNode("retval",node_name);
   }
}

void Solver::handleGetElementPtr(Instruction* I){
  //When occurs a access to a vector, LLVM's IR creat a temporary called "%arayidx". We create a node for this temporary, and set true the 
  //delTemporaryNode flag, thus the algorithm delete this node after the store which use this node. This node points-to the array node.
  Node n;
  Variable v = I->getName();
  n.name = v;
  n.points_to_variables.push_back(v);
  n.next= graph.findNode(I->getOperand(0)->getName());
  graph.insertNode(n,v);
  //Type *p = I->getOperand(0)->getType();
  //errs() << "tipo do argumento " << *p <<"\n";
  //if(p->isStructTy()){
    //errs() <<"e uma struct\n";
     //}
  delTemporaryNode = true;
}

void Solver::handlePHI(Instruction* I){
  //When occurs a phi instruction, LLVM's IR creat a temporary called "%cond". We create a node for this temporary, and set true the 
  //delTemporaryNode flag, thus the algorithm delete this node after the store which use this node. This node points-to the merge of the nodes
  //correspondent to the true and false paths of the condition.
  Node n;
  Variable v = I->getName();
  n.points_to_variables.push_back(v);
  Variable node_true_path, node_false_path;
  node_true_path = I->getOperand(0)->getName();
  node_false_path = I->getOperand(1)->getName();
  //Initially, the node points-to the node of true paths, then, we merge the node of false path in the points-to set.
  if(node_true_path.empty()){
    node_true_path = *(loadVariables.rbegin()+1);
    n.next = graph.findNode(node_true_path)->next;
  }
  else{
    n.next = graph.findNode(node_true_path); 
  }
  if(node_false_path.empty()){
    node_false_path = loadVariables.back();
    errs() <<"NODE FALSE PATH: " <<node_false_path <<"\n";
    errs() << n.next->points_to_variables[0] <<" " <<graph.findNode(node_false_path)->next->points_to_variables[0] <<"\n";
    handleStoreInCopyConstraint(&n, graph.findNode(node_false_path));
  }
  else{
    graph.merge(n.next, graph.findNode(node_false_path));
  }
  graph.insertNode(n,v);
  delTemporaryNode = true;
  copyConstraint = false;
}

bool Solver::runOnModule(Module &M) {
  errs() << "Nome do modulo: " << M.getName() <<"\n";
  for (auto &global : M.getGlobalList()){
    handleAlloca(global.getName());
  }
  for(Function &F: M){
    errs() << "Nome da funcao: " << F.getName() <<"\n";
    for (BasicBlock &bb: F) {
      StringRef bbName = bb.getName();
      if(bbName.startswith("if.then")){
	falseBlock = (bb.getSinglePredecessor()->getName().startswith("if.else") ) ? true : false;
	ifCounter++;
      }
      else{
	falseBlock = (bbName.startswith("if.else")) ? true : false;
	}
      for (Instruction &I: bb) {
	lineno++;
	switch(I.getOpcode()){
	  case Instruction::Alloca:
	    handleAlloca(I.getName());
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
	  case Instruction::Call:
	    handleCall(&I);
	    break;
	  case Instruction::Ret:
	    handleRet(&I);
	    break;
	  case Instruction::GetElementPtr:
	    handleGetElementPtr(&I);
	    break;
	  case Instruction::IntToPtr:
	    handleIntToPtr();
	    break;
	  case Instruction::SExt:
	    ptrOperand = loadVariables.back();
	    break;
	  case Instruction::PHI:
	    handlePHI(&I);
	    break;
	  case Instruction::Br:
	    //graph.print();
	    //if(I.getOperand(0)->getName().startswith("cmp")){
	      //errs() <<"tratar como if\n";
	    //}
	    break;
	}
      }
    }
    ifCounter = 0;
  }
  graph.print();
  return false;
}