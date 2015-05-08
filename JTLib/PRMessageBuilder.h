#ifndef PR_MSGBUILDER_H
#define PR_MSGBUILDER_H


#include "MsgGenerator.h"
#include "SimpleCache.h"

using namespace boost;

class PRMessageBuilder: public MSGGenerator{
public:
	PRMessageBuilder(TreeNode* TN);
	probType buildMsg(Assignment& context,probType lb, DBS& zeroedClauses,Assignment* bestAssignment=NULL);
	//not used in this case
	probType getUpperBound(const Assignment& context,const DBS& zeroedClauses){ 
		return 1.0; 
	}
	void initByTN();
	//nothing to do
	void updateBestAssignment(Assignment& context, DBS& zeroedClauses, Assignment& MPEAss){}

private:
	Cache* cache;
	probType getDNFTermProb(const Assignment& context) const;		
};





#endif