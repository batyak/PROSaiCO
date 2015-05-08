#ifndef NUM_OF_FACTORS_ELIMINATOR_H
#define NUM_OF_FACTORS_ELIMINATOR_H

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "CliqueNode.h"
#include "Eliminator.h"


class NumOfFactorsEliminator: public Eliminator{
private:
	varSet* numOfNodesVec;
	size_t minNumOfNodesIdx;
	size_t maxNumOfNodes;
	unordered_map<varType,size_t> varToNumOfNodes;

	void updateMinIdxUp(); //just deleted the minimum --> minNumOfNodesIdx must increase (or stay the same)
	void updateMinIdxDown(); //just updated the set of nodes a var belongs to --> may affect the index but it can only have gone down
	void updateMinIdx();
public:
	NumOfFactorsEliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, 
							const varToContainingTermsMap& varToContainingTermsOrig);
	virtual varType getNextToEliminate();
	virtual void eliminationCallBack(const varSet& deletedVars, const varSet& affectedVars);
	virtual ~NumOfFactorsEliminator();

};















#endif
