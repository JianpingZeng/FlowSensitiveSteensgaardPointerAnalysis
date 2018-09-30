#include "../include/solver.h"
#include "points_to_graph.cpp"

//Below the methods for handle each LLVM's IR instruction are implemented
void Solver::handleAlloca(Variable node_name){
  Node n;
  Variable v = node_name;
  n.points_to_variables.push_back(v);
  n.next=nullptr;
  graph.insertNode(n,v);
}

void Solver::handleStore(Instruction* I){
  Variable v1 = ((lineno - lineptr == 1) ? ptrOperand : I->getOperand(0)->getName());
  errs() <<"<<<<<<<<<<<<<STORE>>>>>>>>>>>>>>>>>>\n";
  errs() << *I <<"\n";
  errs() <<"0: " <<v1 <<"\n";
  v1 = ((copyConstraint) ? loadVariables.back() : v1);
  errs() <<"1: " <<v1 <<"\n";
  if(v1.empty()){
    Value* op = I->getOperand(0);
     if (ConstantExpr* itpi  = dyn_cast <ConstantExpr>(op)) {
       errs() <<"DEBUG\n";
       v1 = itpi->getAsInstruction()->getOperand(0)->getName();
     }
  }
  Variable v2 = I->getOperand(1)->getName();
  Node *node1 = graph.findNode(v2);
  Node *node2 = graph.findNode(v1);
  //errs() <<"<<<<<<<<<<<<<STORE>>>>>>>>>>>>>>>>>>\n";
  //errs() <<"copyConstraint: "<<copyConstraint <<"\n";
  //errs() <<v2 <<" and " <<v1 <<"\n";
  if(node1!=nullptr && node2!=nullptr){
    if(mayBeExecuted){
      handleStoreInMayExecuteBasicBlock(node1, node2);
      copyConstraint = false;
      return;
    }
    if(delTemporaryNode){ //If we are storing a value from a temporary node, we must delete this node, because it's not exists in the source code
      node1->next = node2->next;
      graph.deleteNode(node2->points_to_variables[0]);
      delTemporaryNode = false;
      return;
    }
    if(node1->next == nullptr){
      (copyConstraint) ? node1->next = node2->next : node1->next = node2;
      copyConstraint = false;
      return;
    }
    //In case of a copy constraint, the node must point to whatever the second node points to.
    (copyConstraint) ? node1->next = node2->next : node1->next = node2;
    //We set the flag to false since we already treated the contraint at this point
    copyConstraint = false;
  }
  
}

void Solver::handleStoreInCopyConstraint(Node* nodeLeft, Node* nodeRight){
  //We create this method because the treatment of a copy instruction is a little bit different.
  nodeLeft->next = nodeRight->next;
}

void Solver::handleStoreInMayExecuteBasicBlock(Node* node1, Node* node2){
  if(node1->next!=nullptr){
    node1->next->points_to_variables.insert(node1->next->points_to_variables.end(),node2->points_to_variables.begin(), node2->points_to_variables.end());
  }
  else{
    Node n;
    n.points_to_variables.push_back(node2->points_to_variables[0]);
    n.next = nullptr;
    Variable node_next_name = to_string(branchCounter);
    graph.insertNode(n, node_next_name);
    node1->next = graph.findNode(node_next_name);
  }
}
  
void Solver::handlePtrToInt(Instruction* I){
  ptrOperand = I->getOperand(0)->getName();
  lineptr = lineno;
}
  
void Solver::handleLoad(Instruction* I){
  loadOperand = I->getOperand(0)->getName();
  if(!loadOperand.empty()){ //If the operand is not a IR's temporary
    loadVariables.push_back(loadOperand);
    //Steensgaard Analysis ignore the statement if the points-to set of the right-side operator is empty
    //In this case, the update it's not necessary
    if(graph.findNode(loadOperand)->next==nullptr) 
      return;
    copyConstraint = true;
  }
}
  
void Solver::handleCall(Instruction* I){
  Function *F = cast<CallInst>(*I).getCalledFunction(); //We cast the instruction to a CallInst to get information about the calee function
  //When occurs a function call that have some kind of return, LLVM's IR store this value in a temporary called %call
  //LLVM's Instruction inherits from Value class, so we can use it's name as a Value.
  if(!I->getName().empty()){ //If the name of the Instruction is not empty, we have a %call temporary receiving the function call
    //A new node is created to this temporary because we need merge it with the variable which receives the function result in the source code
    Node n;
    Variable v = I->getName();
    n.points_to_variables.push_back(v);
    Variable node_next_name = F->getName();
    n.next = &functions[node_next_name]; //It's points-to set is the points-to set of the return node of the function we are handling
    graph.insertNode(n,v);
    copyConstraint = false;
    delTemporaryNode = true;
  }
  unsigned i=F->arg_size();
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
     //We identify the function we are handling, insert it's return node in the functions map, then we delete the retval node from graph.
     Variable node_name = I->getFunction()->getName();
     Node n;
     n.points_to_variables.push_back(node_name);
     n.next = graph.findNode("retval")->next;
     functions[node_name] = n;
     graph.deleteNode("retval");     
   }
}

void Solver::handleGetElementPtr(Instruction* I){
  //When occurs a access to a vector, LLVM's IR creat a temporary called "%arayidx". We create a node for this temporary, and set true the 
  //delTemporaryNode flag, thus the algorithm delete this node after the store which use this node. This node points-to the array node.
  Node n;
  Variable v = I->getName();
  n.points_to_variables.push_back(v);
  n.next= graph.findNode(I->getOperand(0)->getName());
  graph.insertNode(n,v);
  delTemporaryNode = true;
}

void Solver::handleIntToPtr(){
   lineptr = lineno;
   copyConstraint = false;
}

void Solver::handlePHI(Instruction* I){
  //When occurs a phi instruction, LLVM's IR creat a temporary called "%cond". We create a node for this temporary, and set true the 
  //delTemporaryNode flag, thus the algorithm delete this node after the store which use this node. This node points-to the merge of the nodes
  //correspondent to the true and false paths of the condition.
  Node n;
  Variable v = I->getName();
  n.points_to_variables.push_back(v);
  n.next= graph.findNode(I->getOperand(0)->getName()); //Initially, the node points-to the node of true paths
  graph.merge(n.next, graph.findNode(I->getOperand(1)->getName())); //Then, we merge the node of false path in the points-to set.
  graph.insertNode(n,v);
  delTemporaryNode = true;
}

bool Solver::runOnModule(Module &M) {
  errs() << "Nome do modulo: " << M.getName() <<"\n";
  for(auto &global_variable : M.getGlobalList())
    handleAlloca(global_variable.getName());
  for(Function &F: M){
    errs() << "Nome da funcao: " << F.getName() <<"\n";
    for (BasicBlock &bb: F) {
      //The logic below is used to determine if a basicblock is generetade by a conditional statement
      //In another words, it diferentiate a block that "may" be executed from the blocks that "must" be executed
      StringRef bbName = bb.getName();
      if(bbName.startswith("if.then") || bbName.startswith("if.else")){
	mayBeExecuted = true;
	branchCounter++;
      }
      else if (bbName.startswith("sw") && !bbName.endswith("epilog")){
	mayBeExecuted = true;
	branchCounter++;
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
	  case Instruction::Br: //The branch instruction utilizes a operator that is loaded. The load instruction will enable the copyConstraint flag
	    copyConstraint = false; //But in this case we don't have a Copy Constraint, so we set the flag to false
	    break;
	}
      }
    }
    mayBeExecuted = false;
  }
  graph.print();
  return false;
}