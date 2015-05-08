#ifndef CDCL_H
#define CDCL_H

#include "Definitions.h"
#include "CTerm.h"
#include "Definitions.h"
#include "Assignment.h"
#include "LearnedTerm.h"

using namespace std;

class CDCLFormula{
public:
	static long numOfLearnedClauses;
	CDCLFormula(size_t maxNumOfTerms, size_t maxTermSize): maxNumOfTerms(maxNumOfTerms),maxTermSize(maxTermSize){
		learnedTerms.reserve(maxNumOfTerms);				
	}
	
	const termPtrSet& getWatchedTermsByLit(varType lit){
		litToTermPtrSet::const_iterator it = watchedLitToDNFTerms.find(lit);
		if(it == watchedLitToDNFTerms.end()){
			return emptySet;
		}
		return it->second;
	}

	CTerm& getLatestLearnedTerm();

	CTerm* addTerm(const varSet& term,FormulaMgr& FM);
	void deleteTerm(nodeId termId);
	void updateMappedTerms(varType watchedLit);	
	void updateWatchedLiteralsByAssignment(const Assignment& NewlyAssigned, const Assignment& totalAss);
	std::pair<dlevel,dlevel> learnClause(FormulaMgr& FM,const vector<varType>& conflictClause,dlevel currAssLevel, dlevel currNodelevel,varSet& learnedClause);
	bool updateWatchedIdx(CTerm* term, int idx,const Assignment& A);
	void clearCDCL();
	~CDCLFormula();

private:
	static const int m_thresh = 3;  //for m-sized relevance based learning - delete clauses which have more than m unassigned literals
	//CDCL	
	size_t maxTermSize; //maximal size of learned clauses
	dlevel addLearnedClause(const varSet& lits);
	//termSet learnedTerms;	
	termToPtrMap learnedTerms;
	litToTermPtrSet watchedLitToDNFTerms;		
	size_t maxNumOfTerms;
	termPtrSet emptySet;
	std::list<LearnedTerm*> termsByLatestActivation;	
	int impliedCurrLevelVars(const varSet& lits, varSet& retVal,FormulaMgr& FM, dlevel currLevel);
	varType selectLearningVar(const varSet& vars, const varSet& currLearnedClause, FormulaMgr& FM, dlevel currLevel);
	void resolve(const std::vector<varType>& resolvent, varSet& learnedClause, varType lit);

	void mapTermToWatchedLits(LearnedTerm* term,FormulaMgr& FM);

	void eraseTerm(LearnedTerm* term){
		learnedTerms.erase(*term);
		const watchedInfo* wi = term->getWatchedLiterals();
		//watchedLitToDNFTerms[wi[0].watchedLit].erase(term);
		//watchedLitToDNFTerms[wi[1].watchedLit].erase(term);		
		const std::vector<varType> lits=term->getLits();
		for(int i=0 ; i < lits.size() ; i++){
			varType lit = lits[i];
			watchedLitToDNFTerms[lit].erase(term);		
		}
		delete term;
	}

	void eraseTerm(){
		if(!termsByLatestActivation.empty()){
			LearnedTerm* toDelete = termsByLatestActivation.back();
			termsByLatestActivation.pop_back();
			eraseTerm(toDelete);		
		}
	}
	
};



#endif