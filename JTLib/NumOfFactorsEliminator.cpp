#include "Utils.h"
#include "NumOfFactorsEliminator.h"

using namespace boost;

NumOfFactorsEliminator::NumOfFactorsEliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, 
	const varToContainingTermsMap& varToContainingTermsOrig): Eliminator(origCliqueNodes,varToContainingTermsOrig){

	minNumOfNodesIdx = origCliqueNodes.size(); //each var can belong to at most origCliqueNodes.size() terms
	maxNumOfNodes = 0;
	numOfNodesVec = new varSet[origCliqueNodes.size() + 1]; //each var can belong to at most origCliqueNodes.size() terms

	varToContainingTermsMap::const_iterator end = varToContainingTerms.cend();

	for(varToContainingTermsMap::const_iterator mapIt = varToContainingTerms.cbegin() ; mapIt != end ; ++mapIt){
		varType v = mapIt->first;
		const varInfo& vInf = mapIt->second;
		size_t vNumOfTerms = vInf.containingTerms.size();

		numOfNodesVec[vNumOfTerms].insert(v);
		varToNumOfNodes[v] = vNumOfTerms;
		minNumOfNodesIdx = (vNumOfTerms < minNumOfNodesIdx) ? vNumOfTerms : minNumOfNodesIdx;
		maxNumOfNodes = (vNumOfTerms > maxNumOfNodes) ? vNumOfTerms : maxNumOfNodes;
	}
}

varType  NumOfFactorsEliminator::getNextToEliminate(){
	//varType nextToEliminate = *(numOfNodesVec[minNumOfNodesIdx].cbegin());	
	varType nextToEliminate;
	size_t smallestSize = Eliminator::varToContainingTerms.size(); //largest clique node possible
	varSet& possibleVars = numOfNodesVec[minNumOfNodesIdx]; //set of vars belonging to precisely minNumOfNodes
	varSet::const_iterator end = possibleVars.end();
	for(varSet::const_iterator it = possibleVars.begin() ; it != possibleVars.end() ; ++it){
		varType v = *it;
		varInfo& vInf = Eliminator::varToContainingTerms.at(v);
		//calculate the set of uneliminated clique
		varSet unElimClique;
		Utils::intersect(vInf.clique, nonEliminatedVars,unElimClique);
		if(unElimClique.size() < smallestSize){
			smallestSize = unElimClique.size();
			nextToEliminate = v;
		}
	}
	return nextToEliminate;
}

void NumOfFactorsEliminator::eliminationCallBack(const varSet& deletedVars, const varSet& affectedVars){
	varSet::const_iterator delEnd = deletedVars.cend();
	for(varSet::const_iterator delIt = deletedVars.cbegin() ; delIt != delEnd ; ++delIt){
		varType delVar = *delIt;
		size_t delVarIdx = varToNumOfNodes.at(delVar);
		varToNumOfNodes.erase(delVar);
		numOfNodesVec[delVarIdx].erase(delVar);
	}
	
	varSet::const_iterator affEnd = affectedVars.cend();
	for(varSet::const_iterator affIt = affectedVars.cbegin() ; affIt != affEnd ; ++affIt){
		varType affVar = *affIt;

		size_t affVarIdx = varToNumOfNodes.at(affVar); //affected var's former index (num of terms it belongs to)
		size_t newNumOfNodes = varToContainingTerms.at(affVar).containingTerms.size(); //the new num of terms it belongs to
		varToNumOfNodes[affVar] = newNumOfNodes; //update to the new number of terms
		//change position with regard to the new number of nodes the var belongs to
		numOfNodesVec[affVarIdx].erase(affVar); 
		numOfNodesVec[newNumOfNodes].insert(affVar);
	}
	
	if(!nonEliminatedVars.empty()){
		updateMinIdx(); 
	}

}

void NumOfFactorsEliminator::updateMinIdx(){
	int newMin = 1;
	while(numOfNodesVec[newMin].empty()) 
		++newMin;
	minNumOfNodesIdx = newMin;
	
	//this could be the case if all of the variables left are leaves.
	//BOOST_ASSERT_MSG(!cliqueSizeVec[newMin].empty(),Utils::getMsg("cannot find min clique size"));
}

void NumOfFactorsEliminator::updateMinIdxUp(){	
	while(numOfNodesVec[minNumOfNodesIdx].empty()) 
		++minNumOfNodesIdx;
}


void NumOfFactorsEliminator::updateMinIdxDown(){
	int newMin = 1;
	while(numOfNodesVec[newMin].empty()) 
		++newMin;
	minNumOfNodesIdx = newMin;
}


NumOfFactorsEliminator::~NumOfFactorsEliminator(){
	delete[] numOfNodesVec;
}
