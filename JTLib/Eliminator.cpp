#include "Utils.h"
#include "Eliminator.h"

using namespace boost;

void Eliminator::init(){
	nodeId largestId = 0;
	/*
	unordered_map<nodeId,CliqueNode*>::const_iterator end = cliqueNodesMap.cend();
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cit = cliqueNodesMap.cbegin() ; cit != end ; ++cit){
		const varSet& cliqueMembers = cit->second->getVars();
		nodeId id = cit->first;
		largestId = (largestId < id) ? id : largestId;
		varSet::const_iterator varEnd = cliqueMembers.cend();
		for(varSet::const_iterator varIt = cliqueMembers.cbegin(); varIt != varEnd ; ++varIt){
			varInfo& vInf = varToContainingTerms[*varIt];
			vInf.containingTerms.insert(id);
			vInf.clique.insert(cliqueMembers.cbegin(), cliqueMembers.cend());
			vInf.PEOIndex = -1;
		}

	}
	*/
	nonEliminatedVarsBS.resize(FormulaMgr::getInstance()->getNumVars()+1);
	varToContainingTermsMap::const_iterator mapEnd = varToContainingTerms.cend();
	for(varToContainingTermsMap::const_iterator mapIt = varToContainingTerms.cbegin() ; mapIt != mapEnd ; ++mapIt){
		varType v = mapIt->first;		
		nodeIdSet vNodes =  mapIt->second.containingTerms;
		if(vNodes.empty())
			continue;
		nonEliminatedVars.insert(v);	
		nonEliminatedVarsBS[v]=true;
		nodeIdSet::const_iterator end = vNodes.cend();
		varInfo& vInf = varToContainingTerms[v];
		for(nodeIdSet::const_iterator vNodeIt = vNodes.cbegin() ; vNodeIt != end ; ++vNodeIt){
			CliqueNode* vCN = cliqueNodesMap.at(*vNodeIt);
			largestId = (largestId < vCN->getId()) ? vCN->getId() : largestId;
			const varSet& cliqueMembers = vCN->getVars();
			vInf.clique.insert(cliqueMembers.begin(), cliqueMembers.end());
			vInf.PEOIndex = -1;
			/*
			varSet::const_iterator varEnd = cliqueMembers.cend();
			for(varSet::const_iterator varIt = cliqueMembers.cbegin(); varIt != varEnd ; ++varIt){
				varType v_dbg=*varIt;
				varInfo& vInf = varToContainingTerms[*varIt];				
				vInf.clique.insert(cliqueMembers.cbegin(), cliqueMembers.cend());				
				vInf.PEOIndex = -1;
			}*/
		}
	}
	idRunningNum = largestId+1;	
	isAcyclic = true;
	PEO.resize(nonEliminatedVars.size());
	numEliminations = 0;
}

CliqueNode* Eliminator::getMergedNodeIfExists(const varInfo& elimVarInfo , const varSet& mergedVars) const{
	nodeIdSet::iterator end = elimVarInfo.containingTerms.end();
	for(nodeIdSet::const_iterator it = elimVarInfo.containingTerms.cbegin() ; it != end ; ++it){
		CliqueNode* includedNode = cliqueNodesMap.at(*it);
		if(Utils::contains(includedNode->getVars(), mergedVars)){
			return includedNode;		
		}		
	}
	return NULL;
}

void Eliminator::updateMergedNode(CliqueNode* newNode, const nodeIdSet& mergedNodes){
	nodeIdSet::const_iterator end = mergedNodes.cend();
	for(nodeIdSet::const_iterator nodeIt = mergedNodes.cbegin() ; nodeIt != end ; ++nodeIt ){
		CliqueNode* merged = cliqueNodesMap.at(*nodeIt);
		newNode->mergeDNFTerms(*merged);
	}
}

void Eliminator::eliminateVar(){
	numEliminations++;
	varType elimVar = getNextToEliminate();
	#ifdef _DEBUG
		assert(nonEliminatedVars.find(elimVar) != nonEliminatedVars.end());
	#endif
	varInfo& elimVarInf = varToContainingTerms.at(elimVar);
	CliqueNode* mergedNode;
	varSet mergedNodeVars;

	if(elimVarInf.containingTerms.size() == 1){ //belongs to precisely one node
		nodeId mergedNodeId = *(elimVarInf.containingTerms.cbegin());
		mergedNode = cliqueNodesMap.at(mergedNodeId);
		//Utils::intersect(mergedNode->getVars(), nonEliminatedVars, mergedNodeVars);
		Utils::intersect(elimVarInf.clique, nonEliminatedVars, mergedNodeVars);
	}
	else{		
		Utils::intersect(elimVarInf.clique, nonEliminatedVars, mergedNodeVars);
		mergedNode = getMergedNodeIfExists(elimVarInf,mergedNodeVars);
		if(mergedNode == NULL){
			mergedNode = new CliqueNode(getNextNodeId(),mergedNodeVars,false); //not necessarily a DNF term
			cliqueNodesMap[mergedNode->getId()] = mergedNode;	
			isAcyclic = false; //cannot be acyclic, there is no leaf node + merged node does not contain other node.
		}
		updateMergedNode(mergedNode,elimVarInf.containingTerms);
	}
	std::pair<CliqueNodeSet::iterator, bool> insertRes =
		MaximalCliqueNodes.insert(mergedNode); //must be a maximal node because during each elimination all leaves are removed --> no other node can contain all of these leaves	

#ifdef _DEBUG
	if(!insertRes.second){
		string origNodeStr = Utils::printSet((*insertRes.first)->getVars());
		string thisNodeStr = Utils::printSet(mergedNode->getVars());
		int y=9;
	}
	//assert(insertRes.second);
#endif
	

	//delete leaves
	varSet deletedVars;
	DBS deletedVarsBS;
	deletedVarsBS.resize(FormulaMgr::getInstance()->getNumVars()+1);
	varSet remainingVars;
	varSet::const_iterator vEnd = mergedNodeVars.cend();
	for(varSet::const_iterator vIt = mergedNodeVars.cbegin() ; vIt != vEnd ; ++vIt){
		varType v = *vIt;
		varInfo& vInf = varToContainingTerms.at(v);
		//add edges between DNF nodes included in mergedNode
		//addEdgesBetweenContainedDNFNodes(vInf.containingTerms,mergedNode);
		Utils::subt(vInf.containingTerms,elimVarInf.containingTerms);
		removeIfContained(vInf.containingTerms, mergedNode);
		vInf.clique.insert(mergedNodeVars.begin(), mergedNodeVars.end()); //upadate neighbors
		if(vInf.containingTerms.empty()){ //must be a leaf which belongs solely to the new mergedNode --> delete it
			deletedVars.insert(v);		
			PEO.push_back(v);
			deletedVarsBS[v] = true;
		}
		else{
			remainingVars.insert(v);			
		}
	}
	Utils::subt(nonEliminatedVars,deletedVars);
	nonEliminatedVarsBS.operator-=(deletedVarsBS);
	varSet mergedNodeVarsNonElim;//vars in merged node that have not been eliminated
	Utils::intersect(mergedNode->getVars(),nonEliminatedVars,mergedNodeVarsNonElim);

	//now update the neighbors of the deleted vars
	varSet::const_iterator remainingEnd = remainingVars.cend();
	for(varSet::const_iterator remainingIt = remainingVars.cbegin() ; remainingIt != remainingEnd ; ++remainingIt){
		varType t = *remainingIt;
		varInfo& remInf = varToContainingTerms.at(*remainingIt);
		varSet& vReClique = remInf.clique;
		vReClique.insert(mergedNodeVarsNonElim.cbegin(),mergedNodeVarsNonElim.cend()); //added neighbors due to the merge 
				
		//we insert the merged node id to the vars terms iff none of its existing terms contains its un-eliminated neighbors in mergedNode		
		if(!hasContainingNode(remInf.containingTerms,mergedNodeVarsNonElim)){ //all of the non-eliminated vars in mergeNode are neighbors
			remInf.containingTerms.insert(mergedNode->getId());
		}

	}
	eliminationCallBack(deletedVars, remainingVars);
}

void Eliminator::removeIfContained(nodeIdSet& containingNodes, CliqueNode* mergedNode){	
//	DBS intersection;
//	intersection.resize(FormulaMgr::getInstance()->getNumVars()+1);
	for(nodeIdSet::iterator it = containingNodes.begin() ; it != containingNodes.end() ; ){	
		CliqueNode* includedNode = cliqueNodesMap.at(*it);		
		DBS intersection(includedNode->getVarsBS());		
		//intersection.operator|=(includedNode->getVarsBS());
		intersection.operator&=(nonEliminatedVarsBS);
	//	Utils::intersectByLists(includedNode->getVars(), nonEliminatedVars,releventVars);
	//	Utils::intersect(includedNode->getVars(), nonEliminatedVars,releventVars);

		//if(Utils::contains(mergedNode->getVars(), releventVars)){
		if(intersection.is_subset_of(mergedNode->getVarsBS())){
			mergedNode->mergeDNFTerms(*includedNode);
			it = containingNodes.erase(it);		
		}
		else{
			++it;
		}
	}
}

void Eliminator::triangulate(){
	while(!nonEliminatedVars.empty()){
		eliminateVar();
	}
	//overall the tree should contain two kinds of nodes:
	//1. The maximal cliques (are not necessarily DNF terms)
	//2. Nodes representing the DNF terms
	//--> the rest should be removed
	//unordered_map<nodeId,CliqueNode*>::iterator end = cliqueNodesMap.end();
	for(unordered_map<nodeId,CliqueNode*>::iterator cit = cliqueNodesMap.begin() ; cit != cliqueNodesMap.end() ;){
		CliqueNode* node = cit->second;
		if(node->isDNFTerm() || (MaximalCliqueNodes.find(node) != MaximalCliqueNodes.end())){
			++cit;
		}
		else{ //can be a negated node (included in another node), or somelthing else...
			delete node;
			cit = cliqueNodesMap.erase(cit);
		}
	}
}

void Eliminator::addEdgesBetweenContainedDNFNodes(const nodeIdSet& nodeIds, CliqueNode* mergedNode){	
	nodeIdSet::const_iterator end = nodeIds.cend();
	for(nodeIdSet::const_iterator nodeIdIt = nodeIds.cbegin() ; nodeIdIt != end ; ++nodeIdIt){
		CliqueNode* node = cliqueNodesMap.at(*nodeIdIt);
		if(node->isDNFTerm() && Utils::properContains(mergedNode->getVars(), node->getVars())){
			if(mergedNode->getId() < node->getId()){
				treeEdges[mergedNode->getId()].insert(node->getId());
			}
			else{
				treeEdges[node->getId()].insert(mergedNode->getId());
			}
		}		
	}
}


bool Eliminator::hasContainingNode(const nodeIdSet& nodeIds, const varSet& vars){
	nodeIdSet::const_iterator end = nodeIds.cend();
	for(nodeIdSet::const_iterator nodeIdIt = nodeIds.cbegin() ; nodeIdIt != end ; ++nodeIdIt){
		CliqueNode* node = cliqueNodesMap.at(*nodeIdIt);
		if(Utils::contains(node->getVars(), vars)){
			return true;
		}
	}
	return false;
}
