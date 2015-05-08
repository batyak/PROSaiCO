#include "TreeNode.h"
#include <boost/assert.hpp>
#include "Utils.h"
#include "ContractedJunctionTree.h"

using namespace boost;
/*
void MsgNode::initDescendentNodeIds(){
	TreeNode::initDescendentNodeIds();
	//add this node as a descendnt Msg node
	descendentMsgNodeIds.insert(getId());			
}*/


TreeNode* MsgNode::getContainingSibling(const list<TreeNode*>& siblingNodes){
	if(siblingNodes.empty())
		return NULL;
	for(list<TreeNode*>::const_iterator siblingIt = siblingNodes.begin() ; siblingIt != siblingNodes.end() ; ++siblingIt){
		TreeNode* sibling = *siblingIt;
		if(Utils::properContains(sibling->getVars(),this->getVars())){
			return sibling;
		}
	}
	return NULL;
}

probType MsgNode::DNFProb(const Assignment& currAss){
	return 1.0;
}