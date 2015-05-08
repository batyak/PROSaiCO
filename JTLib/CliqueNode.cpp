#include "CliqueNode.h"
#include "ContractedJunctionTree.h"
#include <boost/assert.hpp>
#include "Utils.h"
#include "DirectedGraph.h"
#include "Definitions.h"

using namespace boost;

unordered_map<nodeId,CliqueNode*>  CliqueNode::dnfTermToContainingNode;

TreeNode* CliqueNode::getContainingSibling(const list<TreeNode*>& siblingNodes){
	return NULL; // a clique node is never contained in any other clique node.
}
/*
void CliqueNode::addDNFTerm(CliqueNode* dnfTerm){
	if(Utils::containsAbs(getVars(), dnfTerm->getVars())){
		dnfTerms[dnfTerm->getId()] = dnfTerm;
		dnfTermToContainingNode[dnfTerm->getId()]=this;
	}
}*/

void CliqueNode::mergeDNFTerms(const CliqueNode& otherNode){
	//in this case, otherNode will be separate from this node in the tree
	//therefore, its DNF nodes will be contained to it.
	//TODO: verify if there no bug here!!
	if(!Utils::contains(getVars(), otherNode.getVars())){
		return;
	}
	addDNFTerms(otherNode.getDNFIds(),true);	
}

void CliqueNode::generateDNFLeaves(){	
	clearSubFormula();
	TreeNode::generateDNFLeaves();
	if(isDNFTerm())
		return; //nothing to do!!
	ContractedJunctionTree* CJT = ContractedJunctionTree::getInstance();	
	/*
	const nodeIdSet& dnfTermIds = getDNFIds();
	nodeIdSet::const_iterator end = dnfTermIds.end();*/
	const DBS& dnfTermIds = getDNFIds();
	varSet vars;
	//for(nodeIdSet::const_iterator termId = dnfTermIds.begin() ; termId != end ; ++termId){
	for(size_t termId = dnfTermIds.find_first() ; termId != boost::dynamic_bitset<>::npos ; termId = dnfTermIds.find_next(termId)){
		vars.clear();
		FM.getTerm(termId).getDNFLitSet(vars);		
		nodeId cnId = ContractedJunctionTree::getNextNodeId();
		nodeId msgId = ContractedJunctionTree::getNextNodeId();		
		CliqueNode* cn = new CliqueNode(cnId,vars,true);		
		MsgNode* msg =  new MsgNode(msgId,vars,false);		
		msg->addChild(cn);
		msg->addDNFTerm(termId);
		cn->addDNFTerm(termId);
		cn->setParent(msg);
		addChild(msg);
		msg->setParent(this);		
		CJT->addMsgNode(msg);
		CJT->addCliqueNode(cn);		
	}					
}

void CliqueNode::removeDNFTerm(CliqueNode* dnfTerm){
	dnfTerms.erase(dnfTerm->getId());	
	dnfTermToContainingNode.erase(dnfTerm->getId());
}

probType CliqueNode::DNFProb(const Assignment& GA){	
	if(!isDNFTerm()){ //not  DNF term,cannot be satisfied
		return 1.0;
	}	
	varSet::const_iterator end = vars.end();
	probType satProb = 1.0;
	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		varType v = *it;
		Status vStat = GA.getStatus(v);
		if(vStat == SET_F){ //DNF term must be falsified
			return 1.0;
		}
		if(vStat == UNSET){
			satProb*=FM.getLitProb(v);
		}
	}
	return (1.0 - satProb);	

}