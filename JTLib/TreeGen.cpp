#include "Utils.h"
#include "TreeGen.h"

void TreeGen::getMSCOrder(const varToContainingTermsMap& varToContainingTerms){
	boost::unordered_map<varType, int> varToSetMap; 
	size_t numOfRelevantVars = varToContainingTerms.size();
	PEO = new varType[numOfRelevantVars];
	varSet* setArr = new varSet[numOfRelevantVars]; //setArr[i] contains the set of unnumbered vertices adjacent to exactly i numbered vertices
	for(varToContainingTermsMap::const_iterator it = varToContainingTerms.cbegin(); it != varToContainingTerms.cend() ; ++it){
		setArr[0].insert(it->first); // at first, all nodes are adjacent to 0 numbered nodes
		varToSetMap[it->first]=0;
	}
	int maxSet =0;
	for(size_t i=numOfRelevantVars; i > 0 ; i--){
		varType nextToNumber= *(setArr[maxSet].cbegin());
		setArr[maxSet].erase(nextToNumber);
		PEO[i-1] = nextToNumber;		
		varToPEOIdx[nextToNumber] = (int)i-1;
		const varSet& nbrs = varToContainingTerms.at(nextToNumber).clique;
		varSet::const_iterator nbrsEnd = nbrs.cend();
		for(varSet::const_iterator it = nbrs.cbegin() ; it != nbrsEnd; ++it){ //iterate and update its neighbors
			varType currNbr = *it;
			bool isNumberedNbr = (varToPEOIdx.find(currNbr) != varToPEOIdx.end());
			if(!isNumberedNbr){ //an unnumbered neighbor
				int setArrIdx = varToSetMap[currNbr];
				setArr[setArrIdx].erase(currNbr); // delete from current set in setArr
				varToSetMap[currNbr]=setArrIdx+1;
				setArr[setArrIdx+1].insert(currNbr);
			}
		}
		maxSet++;
		while(setArr[maxSet].empty() && maxSet>0) maxSet--;
	}
	delete[] setArr;	
}

void TreeGen::generateTree(){
	treeEdges.clear();
	varToPEOIdx.clear();

	eliminator->triangulate();
	treeEdges.insert(eliminator->getContainmentEdgeMap().cbegin(), eliminator->getContainmentEdgeMap().cend());
	getMSCOrder(eliminator->getVarInfoMap());
	
/*	vector<varType> PEOVec = eliminator->getPEO();	
	PEO = new varType[PEOVec.size()];
	size_t arrInd = 0;
	size_t vecInd = PEOVec.size()-1;
	while(arrInd < PEOVec.size()){
		PEO[arrInd++]=PEOVec[vecInd--];
	}*/
	
	generateEdges(eliminator->getVarInfoMap());
}

bool TreeGen::validateTree(const EdgesMap& edges){
	EdgesMap cpyMap(edges); //copy of existing edges
	//transform to 2-way map
	for(EdgesMap::const_iterator edgeIt = cpyMap.cbegin() ; edgeIt != cpyMap.cend() ; ++edgeIt){	
		nodeIdSet nbrs = edgeIt->second; //edges from edgeIt->first --> nbrs
		for(nodeIdSet::iterator it = nbrs.begin() ; it != nbrs.end() ; ++it){
			cpyMap[*it].insert(edgeIt->first);			
		}
	}

	list<nodeId> leaves;
	//init leaves for first iteration.
	for(EdgesMap::const_iterator edgeIt = cpyMap.cbegin() ; edgeIt != cpyMap.cend() ; ++edgeIt){	
		if(edgeIt->second.size() == 1){ //node has less thn a single neighbor
			leaves.push_back(edgeIt->first);
		}
	}

	while(!leaves.empty()){
		//remove leaf
		nodeId leaf = leaves.front();
		leaves.pop_front();		
		//get leaf neighbors (can be at most one!!)
		nodeIdSet leafNbr = cpyMap.at(leaf);
		BOOST_ASSERT_MSG(!leafNbr.size() <= 1,Utils::getMsg("leaf can have at most one neighbor"));
		for(nodeIdSet::const_iterator lnit = leafNbr.cbegin() ; lnit != leafNbr.cend() ; ++lnit){
			nodeIdSet& nbrEdges = cpyMap.at(*lnit);
			nbrEdges.erase(leaf);
			if(nbrEdges.size() == 1){ //has become a leaf as well
				leaves.push_back(*lnit);
			}
		}
		cpyMap.erase(leaf); //remove leaf
	}
	if(!cpyMap.empty()){ //there is a set of nodes that all have a degree >=2 --> cannot be a tree
		string s = Utils::printMap(cpyMap);
		return false;
	}
	return true;
}

void TreeGen::generateEdges(const varToContainingTermsMap& varToContainingTerms){
	typedef int cliqueId;
	typedef unordered_set<cliqueId> cliqueIdSet;

	boost::unordered_map<cliqueId,CliqueNode*> tmpCliques;
	boost::unordered_map<varType, cliqueId> lowestCliqueIdMap;
	EdgesMap tempEdges;

	size_t prev_card = 0;
	size_t new_card = 0;
	varSet numberedVertices;
	unsigned int s=0;
	size_t numOfRelevantVars = varToContainingTerms.size();

	for(int i = ((int)numOfRelevantVars - 1); i >= 0 ; i--){
		varType vi=PEO[i];
		varSet vi_Nbs = varToContainingTerms.at(vi).clique;
		varSet vi_NumberedNbs;

		Utils::intersect(vi_Nbs,numberedVertices,vi_NumberedNbs); 		//get the node's numbered neighbors
		new_card = vi_NumberedNbs.size();
		if(new_card <= prev_card){ //begin a new clique
			s++;
			CliqueNode* K_s = new CliqueNode(0,vi_NumberedNbs,false);
			tmpCliques[s]=K_s;

			if(new_card > 0){ //get an edge to the parent clique
				//find min ordered var in Ks
				size_t k = numOfRelevantVars+1;
				for(varSet::const_iterator cit = vi_NumberedNbs.cbegin() ; cit != vi_NumberedNbs.cend() ; ++cit){
					int currOrder = varToPEOIdx[*cit];
					k = k > currOrder ? currOrder : k;
				}
				varType v_k = PEO[k];
				nodeId lowestIdContainingvk = lowestCliqueIdMap[v_k];
				if(s < lowestIdContainingvk){
					tempEdges[s].insert(lowestIdContainingvk);
				}
				else{
					tempEdges[lowestIdContainingvk].insert(s);
				}
			}
		}
		lowestCliqueIdMap[vi] = s; //clique(vi) <-- s
		CliqueNode* lowestCliqueForvi = tmpCliques.at(s);
		lowestCliqueForvi->addVar(vi); // K_s <-- K_s \cup v_i
		numberedVertices.insert(vi); //L_i = L_{i+1} \cup v_i
		prev_card = new_card;

	}
	
	const CliqueNodeSet& maximalCliques = eliminator->getMaxCliques();
	//now translate the edges edges between nodes
	EdgesMap::const_iterator end = tempEdges.cend();
	for(EdgesMap::const_iterator edgeIt = tempEdges.cbegin() ; edgeIt != end ; ++edgeIt){
		CliqueNode* C1_ = tmpCliques.at((const int)edgeIt->first);
		CliqueNode* C1 = *(maximalCliques.find(C1_));
		nodeIdSet::const_iterator nbsEnd = edgeIt->second.cend();
		for(nodeIdSet::const_iterator C1NbsIt = edgeIt->second.cbegin() ; C1NbsIt != nbsEnd ; ++C1NbsIt){
			CliqueNode* C2_ = tmpCliques.at((const int)*C1NbsIt);
			CliqueNode* C2 = *(maximalCliques.find(C2_));
			if(C1->getId() < C2->getId()){
				treeEdges[C1->getId()].insert(C2->getId());
			}
			else{
				treeEdges[C2->getId()].insert(C1->getId());
			}
		}
	}
	//DEBUG
	//test = validateTree(treeEdges);
}
