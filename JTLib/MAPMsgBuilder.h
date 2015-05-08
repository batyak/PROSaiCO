#ifndef MAP_MSGBUILDER_H
#define MAP_MSGBUILDER_H


#include "MPEMsgBuilder.h"
using namespace boost;


class MAPMsgBuilder: public MPEMsgBuilder{
public:
	MAPMsgBuilder(TreeNode* TN):MPEMsgBuilder(TN){}
protected:
	virtual probType getDNFTermMPProb(const Assignment& context, const DBS& irrelevantVars, Assignment* MPEAss = NULL) const;
	virtual probType OptimalAssignmentProb(const DBS& unassignedVars,Assignment* optimalAss = NULL ) const;	
	virtual probType updateIrrelevantVars(const DBS& zereodTerms, DBS& irrelevantVars, Assignment& context);
	//virtual probType updateAssignmentWithIrrelevantVars(const DBS& zereodTerms, const DBS& irrelevantVars,Assignment& context);	
	virtual ~MAPMsgBuilder(){}
};

#endif