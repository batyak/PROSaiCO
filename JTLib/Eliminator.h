#ifndef ELIMINATOR_H
#define ELIMINATOR_H

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "CliqueNode.h"
#include "Definitions.h"



class Eliminator{

private:
	nodeId idRunningNum;
	EdgesMap treeEdges;

public:
	Eliminator(unordered_map<nodeId,CliqueNode*>& origCliqueNodes, const varToContainingTermsMap& varToContainingTermsOrig): 
	  cliqueNodesMap(origCliqueNodes),varToContainingTerms(varToContainingTermsOrig)
	{
		init();
	}
	void eliminateVar();
	void triangulate();
	const EdgesMap& getContainmentEdgeMap(){
		return treeEdges;
	}
	const CliqueNodeSet& getMaxCliques(){
		return MaximalCliqueNodes;
	}
	const unordered_map<nodeId,CliqueNode*>& getCliqueNodesMap(){
		return cliqueNodesMap;
	}

	const varToContainingTermsMap& getVarInfoMap(){
		return varToContainingTerms;
	}

	nodeId getNextNodeId(){
		return ++idRunningNum;
	}

	bool isAcyclicTree(){
		return isAcyclic;
	}

	virtual ~Eliminator(){}
	const vector<varType>& getPEO() const{
		return PEO;
	}
	//DEBUG
	size_t getNumEliminations() const{
		return numEliminations;
	}
	
private:
	void init();
	void addEdgesBetweenContainedDNFNodes(const nodeIdSet& nodeIds, CliqueNode* mergedNode);
	//returns true if one of the members of nodeIds contains all off vars
	bool hasContainingNode(const nodeIdSet& nodeIds, const varSet& vars);
	void removeIfContained(nodeIdSet& containingNodes, CliqueNode* mergedNode);
	CliqueNode* getMergedNodeIfExists(const varInfo& elimVarInfo , const varSet& mergedNodeVars) const;
	bool isAcyclic;
	void updateMergedNode(CliqueNode* newNode, const nodeIdSet& mergedNodes);
	vector<varType> PEO;

	//DEBUG
	size_t numEliminations;
protected:
	varToContainingTermsMap varToContainingTerms;
	unordered_map<nodeId,CliqueNode*>& cliqueNodesMap;
	CliqueNodeSet MaximalCliqueNodes;
	varSet nonEliminatedVars;
	DBS nonEliminatedVarsBS;
	
	void mergeNbs(varType eliminatedVar);
	void eliminateCliqueLeaves(nodeId cliqueContainingLeaves);
	virtual varType getNextToEliminate()=0;
	virtual void eliminationCallBack(const varSet& deletedVars, const varSet& affectedVars) = 0;

	
};















#endif
