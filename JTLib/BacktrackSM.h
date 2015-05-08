#ifndef BACKTRACK_SM
#define BACKTRACK_SM

#include "Definitions.h"
#include "Params.h"
#include "CTerm.h"
#include "FormulaMgr.h"
#include "Assignment.h"

class BacktrackSM{	
public:
	void setBTLevelNode(dlevel nodelevel, dlevel Asslevel, CTerm* lTerm){
		this->nodelevel=nodelevel;		
		this->AssLevel = Asslevel;
		this->lTerm = lTerm;
	}

	bool BT(dlevel currNodeLevel, const Assignment& GA);
	bool correctLearnedClause(varType& latestImpliedLit,const Assignment& GA, FormulaMgr& FM);


	dlevel getBTNodeLevel() const{
		return nodelevel;
	}

	dlevel getBTAssignmentLevel() const{
		return AssLevel;
	}
	bool BacktrackRequired(const Assignment& GA){
		return (Params::instance().CDCL && lTermsSatisfied(GA));
	}

	CTerm* getTerm(){
		return lTerm;
	}

	static BacktrackSM* getInstance();
	static void reset();
private:
	BacktrackSM(){
		nodelevel = 0;
		AssLevel = 0;
		lTerm = NULL;
	}
	bool lTermsSatisfied(const Assignment& GA);
	bool lTermsSatisfiedOrUnit(const Assignment& GA);
	dlevel nodelevel;	
	dlevel AssLevel;
	static BacktrackSM* instance;
	CTerm* lTerm;
	
};


#endif
