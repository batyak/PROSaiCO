#ifndef MSG_NODE_H
#define MSG_NODE_H

#include "TreeNode.h"


class MsgNode: public TreeNode{

public:	
	MsgNode(nodeId id,const varSet& vars, bool isDNFTerm):TreeNode(id,vars,isDNFTerm){}
	virtual TreeNode* getContainingSibling(const list<TreeNode*>& siblingNodes);	
private:

protected:
	//virtual void initDescendentNodeIds();	
	//virtual void initVarToMsgMaps();
	probType DNFProb(const Assignment& currAss);
};

typedef boost::unordered_set<MsgNode*,tn_hash,tn_equal_to> MsgNodeSet;


#endif