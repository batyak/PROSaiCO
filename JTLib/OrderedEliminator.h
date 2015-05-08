#ifndef ORDER_ELIMINATOR_H
#define ORDER_ELIMINATOR_H

#include "Eliminator.h"


class OrderEliminator: public Eliminator{
public:
	OrderEliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, 
				const varToContainingTermsMap& varToContainingTermsOrig, 
				const vector<varType>& eliminationOrder):Eliminator(origCliqueNodes,varToContainingTermsOrig),elimOrder(eliminationOrder),nextInd(0),
		varToContainingTermsOrig(varToContainingTermsOrig){					
	}

	virtual varType getNextToEliminate(){		
		while(irrelevant(nextInd)) 
			nextInd++;		

		varType nextElim = elimOrder[nextInd++];				
		return nextElim;
	}

	virtual void eliminationCallBack(const varSet& _deletedVars, const varSet& affectedVars){
		
	}
private:	
	const vector<varType>& elimOrder;

	bool irrelevant(size_t nextInd){
		varType v = elimOrder[nextInd];
		//return (varToContainingTermsOrig.find(v)==varToContainingTermsOrig.end());
		
		if(nonEliminatedVars.find(v) == nonEliminatedVars.end())
			return true; //already eliminated
		return false;

	}

	size_t nextInd;	
	const varToContainingTermsMap& varToContainingTermsOrig;
};









#endif