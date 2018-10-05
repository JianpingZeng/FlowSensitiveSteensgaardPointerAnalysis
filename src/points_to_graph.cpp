#include "../include/points_to_graph.h"

void Points_to_Graph::insertNode(Node n, Variable node_name){
  nodes[node_name]=n;
  //nodes.insert(pair<Variable, Node>(node_name, n));
}

Node* Points_to_Graph::findNode(Variable node_name){
  // Here we simple check if a node exists in the graph,
  // in positive case we return a reference to it, otherwise we return a null pointer
  return ((nodes.count(node_name) == 1) ? &nodes[node_name] : nullptr);
}

void Points_to_Graph::deleteNode(Variable node_name){
  //This function is used to delete nodes referents to LLVM's IR temporaries, which doesn't exist in the high level code.
  nodes.erase(node_name);
}

void Points_to_Graph::updateNode(Variable node_name, Variable new_node_name){ 
  //This function is used to update a node in the graph. Since the key in C++ map is a const value, we're not able to change it directly,
  //so we have to retrieve the node to be changed, edit what we want (in this case, the name of node), insert this new node and delete the old one.
  Node* node = findNode(node_name);
  node->points_to_variables[0]=new_node_name;
  insertNode(*node, new_node_name);
  nodes.erase(node_name);
}

void Points_to_Graph::merge(Node *n1, Node *n2){
  //This function merge two nodes, doing the union operation of Steensgaard's pointer analysis.
  //We add the variables in points-to set of a node in the points-to set of another one. 
  if(n1==nullptr || n2==nullptr)
    return;
  //errs()<<"merge " <<n1->points_to_variables[0] <<" e " <<n2->points_to_variables[0] <<"\n";
  n1->points_to_variables.insert(n1->points_to_variables.end(), n2->points_to_variables.begin(), n2->points_to_variables.end());
  if(n1->next!=nullptr){  //We merge recursively the nodes pointed by the nodes we first merge
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
      for(unsigned j=0; j<node->points_to_variables.size(); ++j) //Print all variables in a set of a node
	errs() << node->points_to_variables[j] << " "; 
      errs() << "-> ";
      node=node->next;
    }
    errs() <<"\n";
  }
}

/*void Points_to_Graph::print(){
  const StringRef* hj;
  errs() <<"\nfinal graph " <<"\n";
  for(auto &i:nodes){
    hj = &i.first;
    errs() <<*hj<<"\n";
  }
}*/