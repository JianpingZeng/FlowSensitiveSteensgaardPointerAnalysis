#include "../include/points_to_graph.h"

void Points_to_Graph::insertNode(Node n, Variable node_name){
  nodes[node_name]=n;
}

Node* Points_to_Graph::findNode(Variable node_name){
  // here we simple check if a node exists in the graph
  // in positive case we return a reference to it, otherwise we return a null pointer
  return ((nodes.count(node_name) == 1) ? &nodes[node_name] : nullptr);
}

void Points_to_Graph::merge(Node *n1, Node *n2){
  //this function merge two nodes, doing the union operation of Steensgaard's pointer analysis
  //we add the variables in points-to set of a node in the points-to set of another one 
  n1->points_to_variables.insert(n1->points_to_variables.end(), n2->points_to_variables.begin(), n2->points_to_variables.end());
  if(n1->next!=nullptr){  //we merge recursively the nodes pointed by the nodes we first merge
    merge(n1->next, n2->next);
  }
  //we delete the second node recerived, since it was merged into the first one and it's points-to set wasn't lost
  nodes.erase(n2->points_to_variables[0]);
}

void Points_to_Graph::print(){
  Node *node = nullptr;
  errs() << "\nfinal graph " <<"\n";
  for(auto &i: nodes){
    node = &i.second;
    while(node!=nullptr){
      for(unsigned j=0; j<node->points_to_variables.size(); ++j) //print all variables in a set of a node
	errs() << node->points_to_variables[j] << " "; 
      errs() << "-> ";
      node=node->next;
    }
    errs() <<"\n";
  }
}