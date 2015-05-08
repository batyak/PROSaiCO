#include "MsgGeneratorFactory.h"
#include "MPEMsgBuilder.h"
#include "MPEMsgBuilderADD.h"
#include "PRMessageBuilder.h"
#include "TreeNode.h"
#include "Params.h"

void MSGGeneratorFactory::initAlgForTree(TreeNode* root, GenTypes type){
	MSGGenerator* algObj = createObjectByType(root, type);
	if(algObj == NULL){
		return;
	}
	root->setMsgGenerator(algObj);
	algObj->initByTN();
	const list<TreeNode*>& children = root->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){
		initAlgForTree(*child, type);
	}

}


MSGGenerator* MSGGeneratorFactory::createObjectByType(TreeNode* TN, GenTypes type){
	switch(type){
	case MPE:
		return (Params::instance().ADD_LIT_WEIGHTS ? new MPEMsgBuilderADD(TN): new MPEMsgBuilder(TN));
		break;
	case PR:
		return new PRMessageBuilder(TN);
	default:
		cout << " cannot create generator of type " << type;
		return NULL;
	}
}