#ifndef Solver_H
#define Solver_H
#include "points_to_graph.h"

using namespace std;
using namespace llvm;

class Solver{
  //bdd x;
  private:
    Points_to_Graph graph;
    int lineno = 0;
    int lineptr = 0; //stores the line number of a PtrToInt instruction
    Variable ptrOperand; //variable used to store the operand of a PtrToInt instruction
    Variable loadOperand; //variable used to store the operand of a Load instruction
    bool copyConstraint = false; //this flag indicate if we are treating a copy constraint, because it's treatment is different
    
    void handleAlloca(Instruction* I);
    void handleStore(Instruction* I);
    void handleStoreInCopyConstraint(Node* nodeLeft, Node* nodeRight);
    void handlePtrToInt(Instruction* I);
    void handleLoad(Instruction* I);
  
  public:
    bool runOnModule(Module &M); //this method is responsible to do the interface with the LLVM's pass infrastructure
};

#endif