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
  v1 = ((copyConstraint) ? loadVariables.back() : v1);
  if(v1.empty()){ //if the v1 name is empty, we have one instruction as operand of another. We have to convert it and get its operand
    Value* op = I->getOperand(0);
     if (ConstantExpr* itpi  = dyn_cast <ConstantExpr>(op)) {
       v1 = itpi->getOperand(0)->getName();
     }
  }
  Variable v2 = I->getOperand(1)->getName();
  if(v2.empty()){ //if the v2 name is empty, there's a Store instruction with two LLVM's temporaries.	
    if(copyConstraint){ //In this case we have a Load Constraint or a Store Constraint. We treat this as follows:
      //If the storeConstraint flag is true, we set it to false and the first node must points-to the whatever the second node points to, just as 
      //in a Copy Constraint. Otherwise, there's a Load Constraint, thus the first node must points-to the second node itself, just as a Address-Of Constraint
      //We set the copyConstraint flag to false for the function handle the Store Constraint this way. 
      v2 = v1;
      v1 = *++loadVariables.rbegin();
      if(storeConstraint)
	storeConstraint = false;
      else
	copyConstraint = false;
    }
  }
  Node *node1 = graph.findNode(v2);
  Node *node2 = graph.findNode(v1);
  //errs() <<"\n<<<<<<<<<<<<<STORE>>>>>>>>>>>>>>>>>>\n";
  //errs() <<"copyConstraint: "<<copyConstraint <<"\n";
  //errs() <<v2 <<" and " <<v1 <<"\n";
  if(node1!=nullptr && node2!=nullptr){
    if(mayBeExecuted){
      if(!delTemporaryNode){
	handleStoreInMayExecuteBasicBlock(node1, node2);
	copyConstraint = false;
	return;
      }
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
      //graph.print();
      return;
    }
    //In case of a copy constraint, the node must point to whatever the second node points to.
    (copyConstraint) ? node1->next = node2->next : node1->next = node2;
    //We set the flag to false since we already treated the contraint at this point
    copyConstraint = false;
  }
}

void Solver::bindFunctionParameters(Node* actualParameter, Node* formalParameter){ 
  //This function creates a relation between the actual and the formal parameters in a function call
  if(formalParameter->next==nullptr)
    formalParameter->next = actualParameter;
  else
    actualParameter->next = formalParameter->next;
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
  else
    storeConstraint = true;
}
  
void Solver::handleCall(Instruction* I){
  errs() << *I <<"\n";
  Function *F = cast<CallInst>(*I).getCalledFunction();	//We cast the instruction to a CallInst to get information about the calee function
  if(F->getName().startswith("llvm.memcpy")){ //A call to a llvm.memcpy function occurs when there's a Load or Store Constraint involving struct types
    //In this case we use the bitCastOperand variable 
    if(!bitCastOperand.empty()){
      graph.findNode(loadVariables.back())->next = graph.findNode(bitCastOperand);
    }
    else{
      graph.findNode(*++loadVariables.rbegin())->next = graph.findNode(loadVariables.back())->next;
    }
    return;
  }
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
    bindFunctionParameters(graph.findNode(loadVariables[k-i]),graph.findNode(node_name));
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
  GetElementPtrInst* gepi = dyn_cast<GetElementPtrInst>(I);
  Type *t = gepi->getSourceElementType();
  if(t->isStructTy()){ //our analysis isn't field-sensitive, thus we ignore the statements that handle defined types
    errs() <<"e uma struct: "<<I->getName() <<"\n";
    return;
  }
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
  
  //We first create a node to store the temporary "%cond" and insert into the graph
  Node n, n_next;
  Variable node_name = I->getName();
  n.points_to_variables.push_back(node_name);
  n.next = nullptr;
  graph.insertNode(n, node_name);
 
  //Then we create a node for which the "%cond" node must points to. This node must contain the variables correspondent to both true and false paths
  string s1 = I->getFunction()->getName().str();
  const char *c = s1.c_str();
  Twine s2(I->getParent()->getName(), c);
  Variable node_next_name = s2.str();
  unsigned i = I->getNumOperands();
  unsigned k = loadVariables.size();
  Variable true_operand = I->getOperand(0)->getName();
  Variable false_operand = I->getOperand(1)->getName();
  //If the operands of the instruction are temporaries, we need to get the correspondent original variables from the loadVariables array
  (true_operand.empty()) ? n_next.points_to_variables.push_back(loadVariables[k-i]) : n_next.points_to_variables.push_back(true_operand);
  i--;
  (false_operand.empty()) ? n_next.points_to_variables.push_back(loadVariables[k-i]) : n_next.points_to_variables.push_back(false_operand);
  n_next.next = nullptr;
  graph.insertNode(n_next, node_next_name);
  
  //As the nodes were created locally, it is necessary to have them point to the correct location using references to both nodes
  Node* final_n = graph.findNode(node_name); 
  final_n->next = graph.findNode(node_next_name);
  delTemporaryNode = true;
}

void Solver::handleBitCast(Instruction* I){
  bitCastOperand = I->getOperand(0)->getName();
}

bool Solver::runOnModule(Module &M) {
  errs() << "Nome do modulo: " << M.getName() <<"\n";
  for(auto &global_variable : M.getGlobalList())
    handleAlloca(global_variable.getName());
  for(Function &F: M){
    errs() << "Nome da funcao: " << F.getName() <<"\n";
    for (BasicBlock &bb: F) {
      //The logic below is used to determine if a basicblock is generetade by a conditional statement
      //In nother words, it diferentiate a block that "may" be executed from the blocks that "must" be executed
      StringRef bbName = bb.getName();
      if(bbName.startswith("if.then") || bbName.startswith("if.else")){
	mayBeExecuted = true;
      }
      else if (bbName.startswith("sw") && !bbName.endswith("epilog")){
	mayBeExecuted = true;
      }
      for (Instruction &I: bb) {
	lineno++;
	//graph.print();
	//errs() << I <<"\n";
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
	    branchCounter++;
	    break;
	  case Instruction::BitCast:
	    handleBitCast(&I);
	    break;
	}
      }
    }
    mayBeExecuted = false;
  }
  graph.print();
  return false;
}