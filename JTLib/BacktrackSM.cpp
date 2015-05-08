#include "BacktrackSM.h"
#include "CTerm.h"
#include "FormulaMgr.h"
#include "Assignment.h"

BacktrackSM* BacktrackSM::instance=NULL;

void BacktrackSM::reset(){
	delete instance;
	instance = NULL;
}

BacktrackSM* BacktrackSM::getInstance(){
	if(instance == NULL){
		instance = new BacktrackSM();
	}
	return instance;
}

bool BacktrackSM::lTermsSatisfied(const Assignment& GA){
	if(lTerm == NULL){
		return false;
	}	
	const std::vector<varType> lits = lTerm->getLits();		
	for(int i=0 ; i < lits.size() ; i++){
		if(GA.getStatus(lits[i]) != SET_T){
			return false;
		}
	}
	return true;
}

bool BacktrackSM::lTermsSatisfiedOrUnit(const Assignment& GA){
	if(lTerm == NULL){
		return false;
	}	
	int numUnSet=0;
	const std::vector<varType> lits = lTerm->getLits();		
	bool unitOrSAT = true;
	for(int i=0 ; i < lits.size() ; i++){
		varType v = lits[i];
		Status vStat = GA.getStatus(v);		
		if(vStat == SET_F){
			unitOrSAT = false;
			break;
		}
		if(vStat == UNSET){ //there should be at most one unset literal in the newly learned clause				
			numUnSet++;
			if(numUnSet > 1){
				unitOrSAT = false;
				break;
			}
		}
	}
	return unitOrSAT;
}

bool BacktrackSM::BT(dlevel currNodeLevel, const Assignment& GA){
		if(!Params::instance().CDCL || nodelevel == 0 || lTerm==NULL || nodelevel >= currNodeLevel){ //if CDCL not working then never backtrack
			return false;
		}	
		//if here lTerm != NULL && BTLevel < currNodeLevel --> check if learned term is unit or SAT
		bool unitOrSAT = lTermsSatisfiedOrUnit(GA);		
		if(!unitOrSAT){
			lTerm = NULL;
		}
		return unitOrSAT;
	}


