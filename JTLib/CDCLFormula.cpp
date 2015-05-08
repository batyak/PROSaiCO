#include "CDCLFormula.h"
#include "Assignment.h"
#include "BacktrackSM.h"
#include "Utils.h"

long CDCLFormula::numOfLearnedClauses=0;

void CDCLFormula::clearCDCL(){
	learnedTerms.clear();
	termsByLatestActivation.clear();
	watchedLitToDNFTerms.clear();
}

CTerm* CDCLFormula::addTerm(const varSet& termVars,FormulaMgr& FM){	
	LearnedTerm* lTerm = new LearnedTerm();
	lTerm->init(termVars);
	termToPtrMap::iterator termIt = learnedTerms.find(*lTerm);;
	
	if(termIt != learnedTerms.end()){ //term already 		
		return termIt->second;		
	}	
	numOfLearnedClauses++;
	//this is a new term	
	if(learnedTerms.size() >= maxNumOfTerms){
		eraseTerm();
	}	
	learnedTerms[*lTerm]=lTerm;	
	mapTermToWatchedLits(lTerm, FM);
	termsByLatestActivation.push_front(lTerm);
	return termsByLatestActivation.front();
}

CTerm& CDCLFormula::getLatestLearnedTerm(){
	return *termsByLatestActivation.front();
}

void CDCLFormula::mapTermToWatchedLits(LearnedTerm* term,FormulaMgr& FM){	
	//find the two variables with the highest dlevels
	size_t idxMax =0 , idx2sdMax =0;
	dlevel maxLevel = 0, secondMaxLevel = 0;
	for(size_t i = 0; i < term->getLits().size() ; i++){
		if(FM.getVar(term->getLits()[i]).getAssignmentLevel() > maxLevel){
			maxLevel = FM.getVar(term->getLits()[i]).getAssignmentLevel() ;
			idxMax = i;
		}
	}
	for(size_t i = 0; i < term->getLits().size() ; i++){
		if(i == idxMax)
			continue;
		if(FM.getVar(term->getLits()[i]).getAssignmentLevel() > secondMaxLevel){
			secondMaxLevel = FM.getVar(term->getLits()[i]).getAssignmentLevel() ;
			idx2sdMax = i;
		}
	}
	term->setWatched((int)idxMax,0);
	term->setWatched((int)idx2sdMax,1);
	const watchedInfo* wi = term->getWatchedLiterals();
	watchedLitToDNFTerms[wi[0].watchedLit].insert(term);
	watchedLitToDNFTerms[wi[1].watchedLit].insert(term);	
}

bool CDCLFormula::updateWatchedIdx(CTerm* term, int idx,const Assignment& A){
	const watchedInfo* termWI = term->getWatchedLiterals(); //the literals watched in term
	varType prevWatchedLit = termWI[idx].watchedLit; //literal previously watched (which we want to change).
	//DEBUG
	Status prevLitStatus = A.getStatus(prevWatchedLit);
	assert(prevLitStatus==SET_T);	
	int otherWatchedIdx = termWI[(1-idx)].watchedIdx; //the other watched idx (idx \in {0,1})
	const std::vector<varType>&  literals = term->getLits();
	for(size_t i=0 ; i < literals.size() ; i++){ //iterate over all term literals
		if(i != otherWatchedIdx){ //i points to a non-watched lit index
			varType lit = literals[i];
			if(A.getStatus(lit) != SET_T){ 				
				term->setWatched((int)i,idx); //can be set to a new watched literal
				//update watch lists				
				std::pair<termPtrSet::iterator,bool> insertRes = watchedLitToDNFTerms[lit].insert(term); 
				assert(insertRes.second); //make sure that the term was not previously watched by this literal
				termPtrSet& prevPtrSet = watchedLitToDNFTerms.at(prevWatchedLit);
				size_t numErased = prevPtrSet.erase(term);
				assert(numErased==1); //must have been erased				
				return true;
			}			
		}
	}
	return false; //could not find other literal to watch in this term
}

void CDCLFormula::updateMappedTerms(varType watchedLit){
	//a map from a variable to a vector of positions in termsSet
	litToTermPtrSet::iterator termPtrsIt = watchedLitToDNFTerms.find(watchedLit);	
	if(termPtrsIt == watchedLitToDNFTerms.end()){
		return; //already empty.
	}
	unordered_set<CTerm*>& termPositions = termPtrsIt->second;
	for(unordered_set<CTerm*>::iterator termIt = termPositions.begin() ; termIt != termPositions.end();){		
		CTerm* lterm = *termIt;
		const watchedInfo* wi = lterm->getWatchedLiterals();
		watchedLitToDNFTerms[wi[0].watchedLit].insert(lterm);
		watchedLitToDNFTerms[wi[1].watchedLit].insert(lterm);
		if(wi[0].watchedLit != watchedLit && wi[1].watchedLit != watchedLit){			
			termIt = termPositions.erase(termIt);
		}
		else{
			++termIt;
		}			
	}
}

void CDCLFormula::updateWatchedLiteralsByAssignment(const Assignment& NewlyAssigned, const Assignment& totalAss){
	const DBS& n_assignedVars = NewlyAssigned.getAssignedVars();
	const DBS& n_assignment = NewlyAssigned.getAssignment();		
	bool updatedWatched = false;	
	for(size_t i = n_assignedVars.find_first() ; i != boost::dynamic_bitset<>::npos ; i = n_assignedVars.find_next(i)){		
		varType lit = (varType)(n_assignment[i] ? i : 0-i); 		
		litToTermPtrSet::iterator termPtrsIt = watchedLitToDNFTerms.find(lit);	
		if(termPtrsIt == watchedLitToDNFTerms.end()){
			continue; //already empty.
		}
		unordered_set<CTerm*>& termPtrs = termPtrsIt->second;		
		unordered_set<CTerm*>::iterator termPtrsEnd = termPtrs.end();
		for(unordered_set<CTerm*>::iterator termIt = termPtrs.begin() ; termIt != termPtrsEnd; ++termIt){			
			CTerm* currTerm = *termIt;
			updatedWatched |= currTerm->configureWatchedLits(totalAss);	
		}
		if(updatedWatched){
			this->updateMappedTerms(lit);
		}
	}	
}

std::pair<dlevel,dlevel> CDCLFormula::learnClause(FormulaMgr& FM, const std::vector<varType>& conflictClause,
									dlevel assignmentLevel,dlevel nodelevel, varSet& learnedClause){
	varSet currLevelImpliedVars;
	learnedClause.clear();
	learnedClause.insert(conflictClause.begin(), conflictClause.end());	
	int numOfAssignedCurrLevel= impliedCurrLevelVars(learnedClause,currLevelImpliedVars,FM,assignmentLevel);
	bool newClause = false;
	varSet learnedVars;
	while(currLevelImpliedVars.size() > 0){ 
		varType l = selectLearningVar(currLevelImpliedVars,learnedClause,FM,assignmentLevel);		
		CTerm* ant = FM.getVar(l).getAntecendent();
		resolve(ant->getLits(),learnedClause,l);	
		numOfAssignedCurrLevel = impliedCurrLevelVars(learnedClause,currLevelImpliedVars,FM,assignmentLevel);
	}
		
	if(learnedClause.size() == 1){
		varType lit = *learnedClause.begin();
		CVariable& litVar = FM.getVar(lit);
		//in order to fix, we need to return to the point where it was set
		return std::make_pair(litVar.getNodeLevel(), litVar.getAssignmentLevel());
	}
	//if here then learned clause size > 1 + there is at most a single literla from the current assignment level
	//(there could be none)--> assign the clause to the highest node level
	dlevel AssignmentBTLevel=0;
	dlevel nodeBTLevel = 0;	
	varSet::const_iterator end = learnedClause.end();
	for(varSet::const_iterator it = learnedClause.begin() ; it != end ; ++it){
		varType lit = *it;
		CVariable& var = FM.getVar(lit);			
		if(AssignmentBTLevel < var.getAssignmentLevel()){
			AssignmentBTLevel = var.getAssignmentLevel();
			nodeBTLevel = var.getNodeLevel();
		}
	}	

	assert(nodeBTLevel>0);
	assert(AssignmentBTLevel>0);	
	return std::make_pair(nodeBTLevel,AssignmentBTLevel);
}



void CDCLFormula::resolve(const std::vector<varType>& resolvent, varSet& learnedClause,varType lit){
	learnedClause.insert(resolvent.begin(), resolvent.end());
	learnedClause.erase(lit);
	learnedClause.erase(0-lit);	
}

//returns the number of vars assigned at assignment level = currLevel
int CDCLFormula::impliedCurrLevelVars(const varSet& lits, varSet& impliedCurrLevelVars,FormulaMgr& FM, dlevel currLevel){
	int numOfAssignedCurrLevel = 0;
	impliedCurrLevelVars.clear();	
	varSet::const_iterator end = lits.end();
	for(varSet::const_iterator it = lits.begin() ; it != end ; ++it){
		varType lit = *it;
		CVariable& itVar = FM.getVar(lit);
		if(itVar.getAssignmentLevel() == currLevel){
			numOfAssignedCurrLevel++;
			if(itVar.getAntecendent() != NULL){
				impliedCurrLevelVars.insert(lit);
			}
		}		
	}
	return numOfAssignedCurrLevel;
}
//curr strategy - return the implied variable which will create tyhe shortest learned caluse
varType CDCLFormula::selectLearningVar(const varSet& vars,const varSet& currLearnedClause, FormulaMgr& FM, dlevel currLevel){
	varType retVal;
	size_t minSize = 0;
	varSet learnedTemp;
	varSet::const_iterator end = vars.end();
	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		learnedTemp.clear();
		learnedTemp.insert(currLearnedClause.begin(), currLearnedClause.end());
		varType lit = *it;	
		CVariable& lvar = FM.getVar(lit);
		if(lvar.getAntecendent() != NULL && lvar.getAssignmentLevel()==currLevel){ //an implied var at this level
			const std::vector<varType>& currAntLits = lvar.getAntecendent()->getLits();		
			learnedTemp.insert(currAntLits.begin(), currAntLits.end());
			size_t currSize  = learnedTemp.size();
			if(minSize == 0 || minSize > currSize){
				minSize = currSize;
				retVal = lit;
			}		
		}
	}
	return retVal;
}

CDCLFormula::~CDCLFormula(){
	for(list<LearnedTerm*>::iterator itr = termsByLatestActivation.begin() ; itr != termsByLatestActivation.end() ; ){
		LearnedTerm* toDelete = *itr;
		itr = termsByLatestActivation.erase(itr);
		delete toDelete;
	}
	watchedLitToDNFTerms.clear();
	learnedTerms.clear();
	

}
