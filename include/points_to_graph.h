#ifndef Points_to_Graph_H
#define Points_to_Graph_H
#include <vector>

using namespace std;
using namespace llvm;

typedef StringRef Variable;

struct Node{
  vector <Variable> points_to_variables;
  Node *next;
};

class Points_to_Graph{
  map<Variable, Node> nodes;
  
  public:
     void insertNode(Node node, Variable node_name);
     Node* findNode(Variable node_name);
     void merge(Node *n1, Node *n2);
     void print();
     
};

#endif