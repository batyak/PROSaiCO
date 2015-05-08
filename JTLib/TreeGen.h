#ifndef TREE_GEN_H
#define TREE_GEN_H


#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "CliqueNode.h"
#include "Eliminator.h"




class TreeGen{

public:
	TreeGen(Eliminator* eliminator):eliminator(eliminator){
		PEO = NULL;
	}

	void generateTree();
	const EdgesMap& getTreeEdges(){
		return treeEdges;
	}
	static bool validateTree(const EdgesMap& edges);
private:
	Eliminator* eliminator;
	EdgesMap treeEdges;
	varType* PEO;
	boost::unordered_map<varType, int> varToPEOIdx;
	

	void getMSCOrder(const varToContainingTermsMap& varToContainingTerms);
	void generateEdges(const varToContainingTermsMap& varToContainingTerms);
	//DEBUG functions - returns true if the resulting edges form a tree.
	
};












#endif
