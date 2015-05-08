#include "CTerm.h"
#include "Assignment.h"

bool CTerm::DBGIsRelTerm(const Assignment& GA) const{
	for(size_t i=0 ; i < literals.size() ; i++){
		varType lit = literals[i] ;
		if(GA.getStatus(lit) == SET_F){
			return false;
		}
	}
	return true;
	}

void CTerm::setWatchedLits(varType lit1, varType lit2){
	int idx1 = getLitIndex(lit1);
	if(idx1 >= 0){
		setWatched(idx1,0);
	}
	int idx2 = getLitIndex(lit2);
	if(idx2 >= 0){
		setWatched(idx2,1);
	}
}

bool CTerm::satisfiedByAssignment(const Assignment& GA) const{		
	const std::vector<varType>& lits = getLits();		
	for(int i=0 ; i < lits.size() ; i++){
		if(GA.getStatus(lits[i]) != SET_T){
			return false;
		}
	}
	return true;
}

bool CTerm::zeroedByAssignment(const Assignment& GA) const{
	const std::vector<varType>& lits = getLits();		
	for(int i=0 ; i < lits.size() ; i++){
		if(GA.getStatus(lits[i]) == SET_F){
			return true;
		}
	}
	return false;
}

bool CTerm::isUnitByAssignment(const Assignment& GA, varType& unitLit){
	size_t numUnSet = 0;
	const std::vector<varType> lits = getLits();
	for(int i=0 ; i < lits.size() ; i++){
		if(GA.getStatus(lits[i]) == SET_F){
			return false;
		}
		if(GA.getStatus(lits[i]) == UNSET){
			numUnSet++;
			if(numUnSet > 1){
				return false;
			}
			unitLit = lits[i];
		}
	}
	return (numUnSet==1);	
}

Status CTerm::getWatchStatus(const watchedInfo& wf,const Assignment& ass) const{	
	//var not assigned --> cannot be true
	if(!ass.getAssignedVars()[wf.watchedVar]){
		return UNSET;
	}

	const DBS& valAss = ass.getAssignment();
	//either var = 0 and the literal is positive or var = 1 and the literal is negative , either way the watched literal is false
	if((wf.watchedLit > 0 && !valAss[wf.watchedVar]) || (wf.watchedLit < 0 && valAss[wf.watchedVar])){
		return SET_F;
	}
	return SET_T;
}
//AImplied contains both the origional assignment and later implied vars
/*
bool CTerm::isSatisfied(Assignment& AImplied, queue<varType>& Q){
	Status statA = getWatchStatus(watched[0],AImplied);
	switch(statA){
		case SET_F:{
			return false; //clause zeroed out in any case.
				   }
		case UNSET:{
			Status statB = getWatchStatus(watched[1],AImplied);
			if(statB == SET_F || statB == UNSET){ 
				return false; //clause is either zeroed out or there is no implication to add
			}
			//here: statA = UNSET && statB == 1 
			bool updateB = updateWatchedIdx(1,AImplied);
			if(updateB){
				return false; //second watched points to either 0 or unsigned --> no implication
			}
			//watched[1] == 1 && watched[0]=* && cannot find another 0/* literal --> need to zero the literal in watched[0]
			bool toAssign = (watched[0].watchedLit > 0) ? false : true;			
		
			AImplied.setVar(watched[0].watchedVar,toAssign); //toAssign 
			Q.push(toAssign ? watched[0].watchedVar : 0-watched[0].watchedVar);
			return false;						
				   }
		case SET_T:{
			bool updateA = updateWatchedIdx(0,AImplied);
			if(updateA){
				return isSatisfied(AImplied,Q); //call again, this time we know that A points to either 0 or * since the update succeeded				
			}
			//here - B is the only (potentially) unassigned lit left
			Status statB = getWatchStatus(watched[1],AImplied);
			if(statB == SET_F){
				return false; // no implication
			}
			if(statB == SET_T){
				return true; // all literals are set.
			}
			//if here then B = * --> need to imply B
			bool toAssign = (watched[1].watchedLit > 0) ? false : true;		
		
			AImplied.setVar(watched[1].watchedVar,toAssign);
			Q.push(toAssign ? watched[1].watchedVar : 0-watched[1].watchedVar);
			return false;			
				   }

		}

	}*/

bool CTerm::configureWatchedLits(const Assignment& Ass){	
	bool retVal = false;
	if(Ass.getStatus(watched[0].watchedLit) == SET_T){
		retVal |= updateWatchedIdx(0,Ass);
	}
	if(Ass.getStatus(watched[1].watchedLit) == SET_T){
		retVal |= updateWatchedIdx(1,Ass);
	}
	return retVal;
}

bool CTerm::updateWatchedIdx(int idx,const Assignment& A){
	const DBS& assignedVars = A.getAssignedVars();
	const DBS& valAss = A.getAssignment();	
	int otherWatchedIdx = watched[(1-idx)].watchedIdx;
	for(size_t i=0 ; i < literals.size() ; i++){
		if(i != otherWatchedIdx){ //idx points to a non-watched lit
			varType lit = literals[i];
			varType var = (lit > 0) ? lit : 0-lit;
			if(!assignedVars[var] || (lit > 0 && !valAss[var]) || (lit < 0 && valAss[var])){
				setWatched((int)i, idx);
				return true;
			}
		}
	}
	return false;
}

