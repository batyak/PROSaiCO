#ifndef DTREE_H
#define DTREE_H

#include <list>
#include "ElimOrder.h"
#include "Definitions.h"




typedef std::list<varType> List;
typedef std::list<varType>::iterator ListIterator;
typedef std::list<varType>::const_iterator constListIterator;

class DTreeNode {

 public:
  size_t vertex;
  DTreeNode *left;
  DTreeNode *right;
  DTreeNode *parent;
  
  DTreeNode( size_t id, DTreeNode *lt, DTreeNode *rt):
	vertex(id), left(lt), right(rt){
		parent = NULL;
		elim_order = "";
		max_cutset = 0;
		max_context = 0;
		max_cluster =0;
		max_acutset = 0;
		height = 0;
	}
  ~DTreeNode();

  List vars;
  List cutset;
  List acutset;
  List context;
  List cluster;

  string elim_order;  

  size_t max_cutset;
  size_t max_context;
  size_t max_cluster;
  size_t max_acutset;  
  size_t height;

  string treeStats();

  static DTreeNode* generateFromFile(string dtreeFile, string cnfFile);
  static void annotateTree(DTreeNode* d_tree);

};

#endif
