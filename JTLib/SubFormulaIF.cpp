#include "Sub_Formula_IF.h"
#include "CliqueNode.h"
#include "ContractedJunctionTree.h"
#include "Params.h"
#include "BacktrackSM.h"

size_t SubFormulaIF::numOfDeducedTerms=0;
dlevel SubFormulaIF::currNodelevel=0;
boost::unordered_map<dlevel, CDCLFormula*> SubFormulaIF::nodeLevelToCDCL;
termPtrSet SubFormulaIF::emptyTPS;

void SubFormulaIF::updateWatchedLiteralsByAssignment(const Assignment& NewlyAssigned, const Assignment& totalAss){
	const DBS& n_assignedVars = NewlyAssigned.getAssignedVars();
	const DBS& n_assignment = NewlyAssigned.getAssignment();		
	bool updatedWatched = false;	
	for(size_t i = n_assignedVars.find_first() ; i != boost::dynamic_bitset<>::npos ; i = n_assignedVars.find_next(i)){		
		varType lit = (varType)(n_assignment[i] ? i : 0-i); 			
		const nodeIdSet& watchedTerms = FM.getTermIdsByWatchedLit(lit);						
		nodeIdSet::const_iterator watchedEnd = watchedTerms.end();		
		for(nodeIdSet::const_iterator itId = watchedTerms.begin() ; itId != watchedEnd ; ++itId){		
			nodeId termId = *itId;								
			if(!subFormDNFTermIds[termId]){
				continue;
			}
			CTerm& currTerm = FM.getTerm(termId);			
			updatedWatched |= currTerm.configureWatchedLits(totalAss);		
		}
		if(updatedWatched){
			FM.updateMappedTerms(lit);
		}
	}
}
void SubFormulaIF::BCP(Assignment& AssImplied, varType litToSet){
	std::queue<varType> Q;
	Q.push(litToSet);
	satisfiesDescendentDNF(AssImplied,Q, CombinedWeight::getWorstVal(Params::instance().ADD_LIT_WEIGHTS));
}

//assigns unit var and updates level (if required);
void SubFormulaIF::unitAssignVar(CTerm* lterm, Assignment& A, dlevel levelInCaseSingleLit, std::queue<varType>& Q){
	size_t numUnSet = 0;
	varType unitLit;
	size_t unitLitIdx=0;
	dlevel largestSetLevel = 0;
	dlevel largestSetNL = 0;
	const std::vector<varType> lits = lterm->getLits();

	for(int i=0 ; i < lits.size() ; i++){
		if(A.getStatus(lits[i]) == SET_F){
			return; //do nothing
		}
		if(A.getStatus(lits[i]) == UNSET){
			numUnSet++;			
			if(numUnSet > 1){
				return; //not unit - do nothing
			}
			unitLit = lits[i];
			unitLitIdx = i;
		}
		else{ //here lits[i] == SET_T
			dlevel currSetLitLevel = FM.getVar(lits[i]).getAssignmentLevel(); //level of this set literal
			if(largestSetLevel < currSetLitLevel){
				largestSetLevel = currSetLitLevel;
				largestSetNL = FM.getVar(lits[i]).getNodeLevel();
			}
			
		}
	}
	
	if(numUnSet == 1){
		//if here then term has exactly one unset literal
		//A.setVar(unitLit,false); //update assignment
		Q.push(0-unitLit); //this literal will be set --> deal with it during BCP
		//update the unit variable to implied
		CVariable& unitVar = FM.getVar(unitLit);
		unitVar.setAntecendent(lterm);
		if(largestSetLevel > 0){			
			unitVar.setDLevel(largestSetNL,largestSetLevel);			
		}
		else{
			unitVar.setDLevel(nodelevel,levelInCaseSingleLit);
		}
		//update watched lits maps
		//previously watched lits
		/*
		varType w1 = lterm->getWatchedLiterals()[0].watchedLit;
		varType w2 = lterm->getWatchedLiterals()[1].watchedLit;
		if(w1 != unitLit && w2 != unitLit){ //make sure that unitLit is watched		
			updateWatchedIdx(lterm,0,A);					
		}*/
		
	}	
}

bool SubFormulaIF::updateWatchedIdx(CTerm* term, int idx,const Assignment& A){
	if(!term->getIsLearned()){
		return FM.updateWatchedIdx(term,idx,A);
	}
	else /*(Params::instance().CDCL)*/{		
		return CDCLFM->updateWatchedIdx(term,idx,A);
	}
	/*
	if(!term->getIsLearned()){
		return FM.updateWatchedIdx(term,idx,A);
	}
	if(Params::instance().CDCL){		
		return CDCLFM->updateWatchedIdx(term,idx,A);
	}*/
}

bool SubFormulaIF::satisfiesDescendentDNF(Assignment& AssImplied, std::queue<varType>& Q,probType bound){	
	bool retVal = false;	
	//Assignment newAss(AssImplied);
	AssImplied.setOtherAssignment(currGlobal); //newly assigned vars + global	
	dlevel currAssignmentlevel = FM.getVar(Q.front()).getAssignmentLevel();
	probType globalProb=currGlobal.getAssignmentProb().getVal();
	CombinedWeight effectiveBound(globalProb);
	effectiveBound*=(bound);
	if(Params::instance().CDCL){
		CTerm* lTerm = BacktrackSM::getInstance()->getTerm();
		/*
		These two conditions are for the cases in which the latest learned clause
		has been learned during the application of an assignment.
		In this case, the unit literal might not be set by the regular flow.
		We want to avoid the situation in which invalid assignments are repeatedly processed 
		due to the clause learned at the beginning of the assignment paths.
		*/
		if(lTerm != NULL){
			if(lTerm->satisfiedByAssignment(AssImplied)){
				return true;
			}			
			//unitAssignVar(lTerm,AssImplied,currAssignmentlevel,Q);			
		}
	}
	/*
	CDCLFormula* CDCLForm = NULL;
	if(Params::CDCL){
		CDCLForm = getCDCLFM();
	}*/
	/*
	updateWatchedLiteralsByAssignment(newAss, AssImplied);		
	if(CDCLForm != NULL){		
		CDCLForm->updateWatchedLiteralsByAssignment(newAss,AssImplied);
	}	
	*/
	//either retVal == true --> no point in continuing since one of the DNF terms was satisfied
	//or Q is empty --> all needed terms were zeroed out
	while(!Q.empty() && !retVal){ 
		varType litToSet = Q.front();	
		Q.pop();
		AssImplied.setVar(litToSet,true);	
		//check bound	
		CombinedWeight AssWithoutGlob=AssImplied.getAssignmentProb()/globalProb;		
		if(!AssWithoutGlob.betterThanBound(bound)){		
			retVal = true;
			break;
		}

		//retrieve terms watched by this literal
		termPtrVec watchedTerms;
		getAllTermsWatchedByLit(watchedTerms, litToSet);		
		termPtrVec::const_iterator watchedEnd = watchedTerms.end();
		for(termPtrVec::const_iterator itId = watchedTerms.begin() ; itId != watchedEnd ; ++itId){		
			CTerm* currTerm = *itId;
			size_t qSize = Q.size(); //former size
			retVal = isTermSatisfied(currTerm,AssImplied,Q);			
			if(qSize < Q.size()){ //means that the variable at the end of the queue was just assigned due to term currTerm
				varType recentVarSet = Q.back();
				markImpliedVar(recentVarSet,currTerm,currAssignmentlevel);								
			}
			if(retVal) {
				if(Params::instance().CDCL){
					learnClauseProcedure(currTerm,currAssignmentlevel);			
				}
				break;	
			}		

		}	
	}

	AssImplied.clearOtherAssignment(currGlobal);
	return retVal;
}

void SubFormulaIF::learnClauseProcedure(CTerm* currTerm, dlevel currAssignmentlevel ){
	varSet learnedClause;								
	std::pair<dlevel,dlevel> levels = CDCLFM->learnClause(FM,currTerm->getLits(),currAssignmentlevel, nodelevel,learnedClause);
	CDCLFormula* BTForm = getCDCLFormula(levels.first);
	CTerm* learnedCTerm = BTForm->addTerm(learnedClause,FM);
	BacktrackSM::getInstance()->setBTLevelNode(levels.first,levels.second, learnedCTerm); //only place the BTleve can be set										
	if(learnedCTerm->getLits().size() == 1){
		//this is precisely the assignment that failed the formula.
		//two options: either the assignment itself (currNode->v=0) caused
		//it to fail, or an assignment performed during UP
		//also, if here then the size of the learned clause is 1, update the reason for setting
		varType v = learnedCTerm->getLits()[0];
		markImpliedVar(v,learnedCTerm,currAssignmentlevel);
	}
}

size_t SubFormulaIF::estimateLitTermsSize(varType lit) const{
	size_t retVal = 0;
	if(Params::instance().CDCL){
		//CDCLFormula* CDCLForm = getCDCLFM();
		const termPtrSet& learnedWatched =  CDCLFM->getWatchedTermsByLit(lit);
		retVal += learnedWatched.size();
	}
	const termPtrSet& origWatchedTerms = FM.getTermPtrsWatchedLit(lit);
	retVal += origWatchedTerms.size();
	return retVal;
}

//returns all of the term watched by this literal (both origional and learned)
void SubFormulaIF::getAllTermsWatchedByLit(termPtrVec& retVal, varType lit){
	const termPtrSet& learnedWatched = (Params::instance().CDCL) ?  CDCLFM->getWatchedTermsByLit(lit) : emptyTPS;
	const termPtrSet& origWatchedTerms = FM.getTermPtrsWatchedLit(lit);
	size_t requiredSize = learnedWatched.size() + origWatchedTerms.size();
	retVal.reserve(requiredSize);
	retVal.insert(retVal.begin(),learnedWatched.begin(), learnedWatched.end());	
	/*
	size_t requiredSize = estimateLitTermsSize(lit);
	retVal.reserve(requiredSize);
	if(Params::instance().CDCL){		
		const termPtrSet& learnedWatched =  CDCLFM->getWatchedTermsByLit(lit);
		retVal.insert(retVal.begin(),learnedWatched.begin(), learnedWatched.end());		
	}		
	//retrieve terms watched by this literal
	const termPtrSet& origWatchedTerms = FM.getTermPtrsWatchedLit(lit); */	
	termPtrSet::const_iterator end = origWatchedTerms.end();
	for(termPtrSet::const_iterator it = origWatchedTerms.begin() ; it != end ; ++it){
			//nodeId termId = *itId;		
			CTerm* term = *it;
			
			if(!subFormDNFTermIds[term->getId()]){
				continue;
			}
			//CTerm& currTerm = FM.getTerm(termId);		
			retVal.push_back(term);			
	}
	

}
void SubFormulaIF::updateNodeLevel(){
	if(nodelevel <= 0){
		this->nodelevel = ++SubFormulaIF::currNodelevel;
		CDCLFM = new CDCLFormula(subFormDNFTermIds.count()*MULT_COEFF,learnedThresholdCard);		
		nodeLevelToCDCL[nodelevel]=CDCLFM;
	}
	
}

CDCLFormula* SubFormulaIF::getCDCLFormula(dlevel nodeLevel){
	boost::unordered_map<dlevel, CDCLFormula*>::const_iterator it = nodeLevelToCDCL.find(nodeLevel);
	if(it != nodeLevelToCDCL.end()){
		return it->second;
	}
	return NULL;
}
SubFormulaIF::~SubFormulaIF(){
	if(CDCLFM != NULL){
		delete CDCLFM;
	}
	CDCLFM = NULL;
}

bool SubFormulaIF::isTermSatisfied(CTerm* term, Assignment& AImplied, queue<varType>& Q){	
	const std::vector<varType>& lits = term->getLits();
	Status statA =	AImplied.getStatus(term->getWatchedLiterals()[0].watchedLit);
	switch(statA){
		case SET_F:{	
						return false; //clause zeroed out in any case - no mapping changes required
				   }
		case UNSET:{	
						Status statB = AImplied.getStatus(term->getWatchedLiterals()[1].watchedLit);
						if(statB == SET_F || statB == UNSET){ 
							return false; //clause is either zeroed out or there is no implication to add
						}
						//here: statA = UNSET && statB == 1 
						bool updateB = updateWatchedIdx(term,1,AImplied);
						if(updateB){
							return false; //second watched points to either 0 or unsigned --> no implication
						}
						//watched[1] == set && watched[0]=unset && cannot find another 0/* literal --> need to zero the literal in watched[0]
						AImplied.setVar(term->getWatchedLiterals()[0].watchedLit,false);
						Q.push(0-term->getWatchedLiterals()[0].watchedLit); //this literal should be set to true
						return false;
				   }
		case SET_T:{
						bool updateA = updateWatchedIdx(term,0,AImplied); //need to update the watched literal
						if(updateA){
							return isTermSatisfied(term,AImplied,Q); //call again, this time we know that A points to either 0 or * since the update succeeded		
						}
						//here - B is the only (potentially) unassigned lit left
						Status statB =	AImplied.getStatus(term->getWatchedLiterals()[1].watchedLit);
						if(statB == SET_F){
							return false; // no implication
						}
						if(statB == SET_T){							
							return true; // all literals are set.
						}
						//if here then B = * + cannot find another literal that is false or unsigned --> need to imply B
						AImplied.setVar(term->getWatchedLiterals()[1].watchedLit,false);
						Q.push(0-term->getWatchedLiterals()[1].watchedLit); //this literal was set to true
						return false;
				  }
		default:{
					return false; //should never get here.
				}
			 }
	}



	bool SubFormulaIF::DBGtestAssignmentPerSF(const Assignment &GA, const vector<SubFormulaIF*>& subformulas, DirectedGraph& DG) const{
		vector<SubFormulaIF*>::const_iterator end = subformulas.end();
		//test coverage		
		DBS coverageTest;
		coverageTest.resize(subFormDNFTermIds.size(),false);
		coverageTest.reset();
		for(vector<SubFormulaIF*>::const_iterator it = subformulas.begin() ; it != end ; ++it){
			SubFormulaIF* currSF = *it;
			coverageTest.operator|=(currSF->getDNFIds());
		}
		//test
		if(coverageTest!=subFormDNFTermIds){
			cout << " coverage test failed " << endl ; 
			return false;
		}
		//get the relevant vars for each child.
		std::vector<DBS> childrenRelevantVars;
		childrenRelevantVars.reserve(subformulas.size());		
		int childIdx = 0;
		//get the relevant variables per each subformula
		for(vector<SubFormulaIF*>::const_iterator it = subformulas.begin() ; it != end ; ++it,childIdx++){
			childrenRelevantVars.push_back((*it)->DBGGetRelevantVars(GA));			
		}
		//test that no two subformulas intersect
		for(int i=0 ; i < childrenRelevantVars.size() ; i++){
			for(int j=i+1 ; j < childrenRelevantVars.size() ; j++){
				bool ijIntersect = childrenRelevantVars[i].intersects(childrenRelevantVars[j]);
				if(ijIntersect){
					DBS ijIntersection = childrenRelevantVars[i];
					ijIntersection.operator&=(childrenRelevantVars[j]);
					cout << " The following two subformulas intersect given assignment " << GA.print() << ":" << endl <<
						subformulas[i]->DBGPrintSubFormula() << " and " << subformulas[j]->DBGPrintSubFormula();
					cout << " The intersecting variables and their incoming bitsets are: " << endl;
					for(size_t idx = ijIntersection.find_first() ; idx != DBS::npos ; idx = ijIntersection.find_next(idx)){
						cout << endl;
						cout << idx << ":" << " ";
						DBS iOut = DG.getOutgoingBS((varType)idx);
						for(size_t onv = iOut.find_first() ; onv != DBS::npos ; onv = iOut.find_next(onv)){
							varType outVar = DG.getVarByIdx(onv);
							cout << outVar << ",";
						}
					}
					//now print the incoming edges to assigned vars in the graph.
					cout << endl << " Incoming edges to assigned literals:";
					const varSet& DGVars = DG.getVertices();
					for(varSet::const_iterator vIt = DGVars.cbegin() ; vIt != DGVars.cend() ; ++vIt){
						if(GA.getStatus(*vIt) == UNSET)
							continue;

						varType lit = (GA.getStatus(*vIt) == SET_F) ? *vIt : 0-*vIt;
						cout << endl << lit << ": ";
						DBS inLit = DG.getIncomingBS(lit);
						for(size_t inLitIdx = inLit.find_first() ; inLitIdx != DBS::npos ; inLitIdx = inLit.find_next(inLitIdx)){
							varType inVar = DG.getVarByIdx(inLitIdx);
							cout << inVar << ",";
						}
					}
						
					
					cout << endl;
					cout << " The directed graph: " << DG.printGraph();
					return false;

				}
			}
		}
		return true;
	}