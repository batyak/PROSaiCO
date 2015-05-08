#ifndef MSG_GEN_FACTORY_H
#define MSG_GEN_FACTORY_H

class MSGGenerator;
class TreeNode;

class MSGGeneratorFactory{

public:
	enum GenTypes{
		MAP,
		MPE,
		PR
	};
	MSGGeneratorFactory(){}
	void initAlgForTree(TreeNode* root, GenTypes type);

private:
	MSGGenerator* createObjectByType(TreeNode* TN, GenTypes type);
};









#endif