#include "Utils.h"
#include "CliqueSizeEliminator.h"
#include <boost/assert.hpp>

using namespace boost;

CliqueSizeEliminator::CliqueSizeEliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, 
	const varToContainingTermsMap& varToContainingTermsOrig): Eliminator(origCliqueNodes,varToContainingTermsOrig){

	minSizeIdx = varToContainingTerms.size(); //each var can have at most varToContainingTerms.size()-1 neighbors
	maxCliqueSize = 0;
	cliqueSizeVec = new varSet[varToContainingTerms.size() + 1]; //each var can have a clique of at most  varToContainingTerms.size()

	varToContainingTermsMap::const_iterator end = varToContainingTerms.cend();

	for(varToContainingTermsMap::const_iterator mapIt = varToContainingTerms.cbegin() ; mapIt != end ; ++mapIt){
		varType v = mapIt->first;
		const varInfo& vInf = mapIt->second;
		size_t vCliqueSize = vInf.clique.size();
		if(vInf.containingTerms.size() == 1){
			leaves.insert(v);			
		}
		else{			
			cliqueSizeVec[vCliqueSize].insert(v);
			varToCliqueSize[v] = vCliqueSize;
			minSizeIdx = (vCliqueSize < minSizeIdx) ? vCliqueSize : minSizeIdx;
		    maxCliqueSize = (vCliqueSize > maxCliqueSize) ? vCliqueSize : maxCliqueSize;
		}
		
	}
}


varType  CliqueSizeEliminator::getNextToEliminate(){
	if(!leaves.empty()){
		return *(leaves.cbegin());
	}
	return *(cliqueSizeVec[minSizeIdx].cbegin());
}

void CliqueSizeEliminator::eliminationCallBack(const varSet& deletedVars, const varSet& affectedVars){
	varSet::const_iterator delEnd = deletedVars.cend();
	for(varSet::const_iterator delIt = deletedVars.cbegin() ; delIt != delEnd ; ++delIt){
		varType delVar = *delIt;
		if(leaves.find(delVar) != leaves.end()){
			leaves.erase(delVar);
			continue;
		}
		size_t delVarIdx = varToCliqueSize.at(delVar); //var's clique size
		varToCliqueSize.erase(delVar);
		cliqueSizeVec[delVarIdx].erase(delVar);	 //remove from clique size index
	}
	
	varSet::const_iterator affEnd = affectedVars.cend();
	for(varSet::const_iterator affIt = affectedVars.cbegin() ; affIt != affEnd ; ++affIt){
		varType affVar = *affIt;
		varInfo& affVarInf = varToContainingTerms.at(affVar);		
		size_t newNumOfNodes = affVarInf.containingTerms.size(); //the new num of terms it belongs to

		if(newNumOfNodes == 1){ //became a leaf
			transformToLeaf(affVar);			
		}
		else{
			//calculate the set of uneliminated clique
			varSet unElimClique;
			Utils::intersect(affVarInf.clique, nonEliminatedVars,unElimClique);

			size_t affVarIdx = varToCliqueSize.at(affVar); //affected var's former index (size of clique it belongs to)
			size_t newCliqueSize = unElimClique.size();
			varToCliqueSize[affVar] = newCliqueSize; //update map from var to clique size
			cliqueSizeVec[affVarIdx].erase(affVar); //update clique size index
			cliqueSizeVec[newCliqueSize].insert(affVar);
		}		
	}
	if(!leaves.empty()){ //next call to getNextToEliminate will return a leaf
		return;
	}

	if(!nonEliminatedVars.empty()){
		updateMinIdx(); 
	}

}

void CliqueSizeEliminator::transformToLeaf(varType var){
	size_t varIdx = varToCliqueSize.at(var); //affected var's former index (size of clique it belongs to)
	varToCliqueSize.erase(var);
	cliqueSizeVec[varIdx].erase(var);
	leaves.insert(var);
}

void CliqueSizeEliminator::updateMinIdxUp(){	
	while(cliqueSizeVec[minSizeIdx].empty()) 
		++minSizeIdx;
}


void CliqueSizeEliminator::updateMinIdx(){
	int newMin = 1;
	while(cliqueSizeVec[newMin].empty()) 
		++newMin;
	minSizeIdx = newMin;
	
	//this could be the case if all of the variables left are leaves.
	//BOOST_ASSERT_MSG(!cliqueSizeVec[newMin].empty(),Utils::getMsg("cannot find min clique size"));
}



CliqueSizeEliminator::~CliqueSizeEliminator(){
	delete[] cliqueSizeVec;
}

