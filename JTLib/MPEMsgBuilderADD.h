#ifndef MPE_MSGBUILDER_ADD_H
#define MPE_MSGBUILDER_ADD_H
#include "MPEMsgBuilder.h"

const probType ADD_DBL_EPSILON=2.2204460492503131e-016; /* smallest such that 1.0+DBL_EPSILON != 1.0 */
class MPEMsgBuilderADD: public MPEMsgBuilder{
public:		
	MPEMsgBuilderADD(TreeNode* TN):MPEMsgBuilder(TN){}

protected:	
	virtual ~MPEMsgBuilderADD(){}
private:
	//returns true if p1 is preferred over p2
	virtual inline bool compareLitProbs(probType p1, probType p2) const{
		return (p1 <= p2);
	}

	//returns true if a1 is preferred over a2
	virtual inline bool compareAssignmentProbs(probType a1, probType a2) const{
		return (a1 < a2);
	}

	virtual inline probType worstZeroOneRatio() const{
		return INT_MAX; //under the assumption that we want the smallest subtraction
	}
	
	virtual inline probType computeRatio(probType negProb, probType posProb) const{
		return (negProb - posProb); //in ADD case, should be negProb - posProb
	}

	virtual inline probType worstRetVal(probType bound) const{
		return bound+1; //in ADD case, return a result a bit worse than the bound
	}


	virtual inline bool compareRatios(probType r1, probType r2) const{
		return (r1 < r2);
	}

	virtual probType childrenUpperBounds(const Assignment& context){
		CombinedWeight baseVal;
		probType val = baseVal.getVal();
		const list<TreeNode*>& children = TN->getChildren();
		list<TreeNode*>::const_iterator childEnd = children.end();			
		for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){
			(*child)->upperBound =val;
		}

		return val;
	}

	virtual void calculategenericUpperBounds(TreeNode* node){
		CombinedWeight UPCW;
		node->genericUP=UPCW.getVal();
	}
	
	//ASSUMPTION: we are in MAX-SAT solving mode
	//therefore the optimal prob is ALWAYS 0 and is received when setting vars to T
	//-->simply set all vars to false
	virtual probType OptimalAssignmentProb(const DBS& unassignedVars,Assignment* optimalAss = NULL ) const{
		CombinedWeight retVal;
		if(optimalAss != NULL){
			optimalAss->setAllToFalse(unassignedVars);
		}
		return retVal.getVal();
	}
};

#endif




