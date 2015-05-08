#ifndef CLIQUE_NODE_H
#define CLIQUE_NODE_H

#include "TreeNode.h"
#include "Utils.h"
#include "Definitions.h"


class CliqueNode: public TreeNode{

public:
	typedef boost::unordered_map<nodeId, CliqueNode*> idToCliqueNodePtrMap;

	static unordered_map<nodeId,CliqueNode*> dnfTermToContainingNode;

	CliqueNode(nodeId id,varSet& vars, bool isDNFTerm):TreeNode(id,vars,isDNFTerm){}

	virtual TreeNode* getContainingSibling(const list<TreeNode*>& siblingNodes);

	void generateDNFLeaves();

	void addVar(varType v){
		this->vars.insert(v);
	}

	void removeDNFTerm(CliqueNode* dnfTerm);

	size_t numOfContainedTerms() const{
		return dnfTerms.size();
	}
	void mergeDNFTerms(const CliqueNode& otherNode);
	

	const idToCliqueNodePtrMap& getDnfTermConstraints() const{
		return dnfTerms;
	}	
protected:	
	probType DNFProb(const Assignment& currAss);
private:
	bool satisfyDNFNode(CliqueNode* dnfNode, const Assignment& Ass) const;

	idToCliqueNodePtrMap dnfTerms;
};

typedef boost::unordered_set<CliqueNode*,tn_hash,tn_equal_to> CliqueNodeSet;









#endif