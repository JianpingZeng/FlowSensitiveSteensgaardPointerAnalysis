#ifndef Solver_H
#define Solver_H
#include "points_to_graph.h"

using namespace std;
using namespace llvm;

class Solver{
  private:
    Points_to_Graph graph;
    int lineno = 0;
    int lineptr = 0; //stores the line number of a PtrToInt instruction
    Variable ptrOperand; //variable used to store the operand of a PtrToInt instruction
    Variable loadOperand; //variable used to store the operand of a Load instruction
    vector<Variable> loadVariables; //vector used to store the operands of the load instructions. It is necessary being a vector because the function calls
    bool copyConstraint = false; //this flag indicate if we are treating a copy constraint, because it's treatment is different
    bool delTemporaryNode = false; //this flag indicate a Store with a node correspondent to a LLVM's IR temporary. This means we need delete the node after the store
    
    void handleAlloca(Instruction* I);
    void handleStore(Instruction* I);
    void handleStoreInCopyConstraint(Node* nodeLeft, Node* nodeRight);
    void handlePtrToInt(Instruction* I);
    void handleLoad(Instruction* I);
    void handleCall(Instruction* I);
    void handleRet(Instruction* I);
    void handleGetElementPtr(Instruction* I);
    void handlePHI(Instruction* I);
  
  public:
    bool runOnModule(Module &M); //this method is responsible to do the interface with the LLVM's pass infrastructure
};

#endif