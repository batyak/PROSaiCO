#ifndef CLIQUE_SIZE_ELIMINATOR_H
#define CLIQUE_SIZE_ELIMINATOR_H

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "CliqueNode.h"
#include "Eliminator.h"



class CliqueSizeEliminator: public Eliminator{
private:
	varSet* cliqueSizeVec;
	size_t minSizeIdx;
	size_t maxCliqueSize;
	varSet leaves; //are always first
	unordered_map<varType,size_t> varToCliqueSize;
	void transformToLeaf(varType var);

	void updateMinIdxUp(); //just deleted the minimum --> minNumOfNodesIdx must increase (or stay the same)
	void updateMinIdxDown(); //just updated the var's neighbors --> may affect the index but it can only have gone down
	void updateMinIdx(); //just searches for the minimum

public:
	CliqueSizeEliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, 
				const varToContainingTermsMap& varToContainingTermsOrig);
	virtual varType getNextToEliminate();
	virtual void eliminationCallBack(const varSet& deletedVars, const varSet& affectedVars);
	virtual ~CliqueSizeEliminator();
};















#endif
