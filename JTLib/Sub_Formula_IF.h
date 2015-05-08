#ifndef SUB_FORM_IF
#define SUB_FORM_IF

#include "Assignment.h"
#include "FormulaMgr.h"
#include "SimpleCache.h"
#include "CDCLFormula.h"
#include "Params.h"
#include "DirectedGraph.h"
#include <queue>

class CliqueNode;
class CTermPQCompare{
private:
	FormulaMgr& FM;
public:
	CTermPQCompare(FormulaMgr& FM_):FM(FM_){}
	
	bool operator() (nodeId lid, nodeId rid) const{		
		const CTerm& lTerm = FM.getTerm(lid);
		const CTerm& rTerm = FM.getTerm(rid);
		return (lTerm.getNumLits() < rTerm.getNumLits());
	}

};

class SubFormulaIF{	
public:
	static dlevel currNodelevel;
	static size_t numOfDeducedTerms;	
	virtual bool satisfiesDescendentDNF(Assignment& AssImplied, std::queue<varType>& Q, probType bound);	
	void BCP(Assignment& AssImplied, varType litToSet);
	
	virtual ~SubFormulaIF();
	
	const DBS& getDNFIds() const{
		if(isTerm){
			return FM.getEmptyTermsDBS();
		}
		return subFormDNFTermIds;
	}

	DBS* getDNFIdsPtr(){
		return &subFormDNFTermIds;
	}
	string DBGPrintSubFormula() const{
		std::stringstream ss;
		for(size_t i = subFormDNFTermIds.find_first() ; i != DBS::npos ; i = subFormDNFTermIds.find_next(i)){			
			CTerm& term = FM.getTerm(i);
			const vector<varType>& vec = term.getLits();
			std::copy(vec.begin(), vec.end()-1,
				std::ostream_iterator<varType>(ss, "."));
			ss << vec.back();
			ss << endl;
		}
		return ss.str();	
	}
	/*test function:
	verifies that:
	1. the children terms cover the subformula terms
	2. Given the assignment GA, the subformulas are disjoint 
	*/
	bool DBGtestAssignmentPerSF(const Assignment &GA, const vector<SubFormulaIF*>& subformulas, DirectedGraph& DG) const;
	

	//get variables relevant for this subformula
	DBS DBGGetRelevantVars(const Assignment &GA){
		//initialize
		DBS relVars(subtreeVars.size());		
		//iterate over subformula variables
		for(size_t i = subFormDNFTermIds.find_first() ; i != DBS::npos ; i = subFormDNFTermIds.find_next(i)){
			CTerm& iterm = FM.getTerm(i);
			//returns true if this term is not unSAT
			if(iterm.DBGIsRelTerm(GA)){
				const std::vector<varType>& lits = iterm.getLits();
				for(int j=0 ; j < lits.size() ; j++){
					varType v = lits[j] > 0 ? lits[j] : 0-lits[j];
					relVars.set(v,true); //variable in an unSAT term
				}
			}
		}
		//remove assigned terms
		relVars.operator-=(GA.getAssignedVars());
		return relVars;
	}

	void addDNFTerm(nodeId id, bool organic=false){		
		if(isTerm)
			return;
		subFormDNFTermIds[id]=true;
		if(organic){
			organicDNFTermIds.insert(id);
		}
		CTerm& currTerm = FM.getTerm(id);		
		//set appropriate vars in the DBS
		const std::vector<varType>& lits = currTerm.getLits();
		std::vector<varType>::const_iterator end = lits.end();
		for(std::vector<varType>::const_iterator it = lits.begin() ; it != end ; ++it){
			varType v = *it;
			v = (v < 0) ? (0-v):v; //here v>=0
			subtreeVars[v]=true;
		}
		learnedThresholdCard = (currTerm.getNumLits() > learnedThresholdCard) ? currTerm.getNumLits(): learnedThresholdCard;	
		//mapVarsToTerm(id, currTerm);
	}

	void addDNFTerms(const nodeIdSet& ids,bool organic=false){
		nodeIdSet::const_iterator end = ids.end();
		for(nodeIdSet::const_iterator tid = ids.begin(); tid != end ; ++tid){
			addDNFTerm(*tid,organic);
		}
	}

	void addDNFTerms(const DBS& ids,bool organic=false){
		for(size_t i = ids.find_first() ; i != boost::dynamic_bitset<>::npos ; i = ids.find_next(i)){
			addDNFTerm(i,organic);
		}		
	}

	FormulaMgr& getFM(){
		return FM;
	}
	dlevel getNodeLevel() const{
		return nodelevel;
	}

	void setDecisionVar(varType lit, dlevel varAssLevel){
		CVariable& justSetVar =FM.getVar(lit); 
		justSetVar.setAntecendent(NULL);
		justSetVar.setDLevel(nodelevel,varAssLevel);		
	}

	void markImpliedVar(varType lit, CTerm* antecendent, dlevel currAssignmentlevel){
		CVariable& justSetVar =FM.getVar(lit); 
		justSetVar.setAntecendent(antecendent);
		justSetVar.setDLevel(nodelevel,currAssignmentlevel);
	}

	void markLiteralUnSet(varType lit){
		CVariable& justSetVar =FM.getVar(lit); 
		justSetVar.setAntecendent(NULL);
		justSetVar.setDLevel(0,0);
	}

	const DBS& getSubtreeVars() const{		
		if(isTerm){
			return FM.getEmptyVarsDBS();
		}
		return subtreeVars;
	}
	/*
	const varToBS& getVarToSubFormulaTerms(){
		return varToSubFormulaTermsBS;
	}*/
	void updateNodeLevel();

	virtual void updateGlobalAssignment(const Assignment& GA_){
		currGlobal.clearAssignment();
		currGlobal.setOtherAssignment(GA_);				
	}	
protected:

	SubFormulaIF(FormulaMgr& FM_, bool isTerm_):FM(FM_),comparator(FM_),isTerm(isTerm_){
		subFormDNFTermIds.resize(isTerm ? 0 : FM.getNumTerms());
		learnedThresholdCard = 0;		
		CDCLFM = NULL;
		nodelevel = 0;				
		subtreeVars.resize(isTerm ? 0 : FM.getNumVars()+1);		
		subtreeVars.reset();
		ADD=Params::instance().ADD_LIT_WEIGHTS;
	}

	

	FormulaMgr& FM;
	Assignment currGlobal;		
	unordered_map<nodeId,CliqueNode*> learnedClauses;		
	

	bool operator() (const nodeId& lhs, const nodeId& rhs) const{
		const CTerm& lTerm = FM.getTerm(lhs);
		const CTerm& rTerm = FM.getTerm(rhs);
		return (lTerm.getNumLits() < rTerm.getNumLits());
	}	

	void clearSubFormula(){
		nodelevel = 0;
		subFormDNFTermIds.reset();	
		//re-add the organic dnf terms
		addDNFTerms(organicDNFTermIds,false);
		if(CDCLFM != NULL){
			delete CDCLFM;
			CDCLFM = NULL;
		}
	}

	void getDNFTerms(nodeIdSet& retVal) const{
		for(size_t i = subFormDNFTermIds.find_first() ; i != boost::dynamic_bitset<>::npos ; i = subFormDNFTermIds.find_next(i)){
			retVal.insert(i);
		}
	}

	
	static CDCLFormula* getCDCLFormula(dlevel nodelevel);	
	varType getlatestImpliedLiteral(const Assignment& GA);
	dlevel nodelevel;	
	DBS subtreeVars;

	
private:	
	//nodeIdSet subFormDNFTermIds;		
	DBS subFormDNFTermIds;
	nodeIdSet organicDNFTermIds;			
	CTermPQCompare comparator;
	static const size_t MULT_COEFF = 1;
	std::vector<nodeId> heapVecTermIds;
	nodeIdSet deducedTermIds;
	bool isTerm;
	//returns all of the term watched by this literal (both origional and learned)
	void getAllTermsWatchedByLit(termPtrVec& retVal, varType lit);

	void updateWatchedLiteralsByAssignment(const Assignment& NewlyAssigned, const Assignment& totalAss);
	
	typedef boost::unordered_map<dlevel, CDCLFormula*> dLevelToCDCLFormulaMap;
	//CDCL	
	size_t learnedThresholdCard;
	CDCLFormula* CDCLFM;	
	void unitAssignVar(CTerm* lterm, Assignment& A, dlevel levelInCaseSingleLit,std::queue<varType>& Q);
	static boost::unordered_map<dlevel, CDCLFormula*> nodeLevelToCDCL;		
	static termPtrSet emptyTPS;
	/*
	CDCLFormula* getCDCLFM(){
		if(!Params::CDCL){
			return NULL;
		}
		if(CDCLFM != NULL){
			return CDCLFM;
		}
		CDCLFM = new CDCLFormula(subFormDNFTermIds.count()*MULT_COEFF,learnedThresholdCard);
		return CDCLFM;
	}*/

	bool updateWatchedIdx(CTerm* term, int idx,const Assignment& A);
	bool isTermSatisfied(CTerm* term, Assignment& AImplied, queue<varType>& Q);

	/*
	varToBS varToSubFormulaTermsBS;

	void mapVarsToTerm(nodeId termId, const CTerm& term){
		const std::vector<varType>& lits = term.getLits();
		std::vector<varType>::const_iterator end = lits.end();
		for(std::vector<varType>::const_iterator it = lits.begin() ; it != end ; ++it){
			varType v = *it;
			varToBS::iterator vit = varToSubFormulaTermsBS.find(v);
			if(vit == varToSubFormulaTermsBS.end()){
				varToSubFormulaTermsBS[v].resize(FM.getNumTerms());	
			}
			varToSubFormulaTermsBS[v].set(termId,true);			
		}
	}*/

	size_t estimateLitTermsSize(varType lit) const;
	bool ADD;
	inline bool betterThanBound(probType p, probType bound){
		bool retVal = (ADD ? p <= bound : p >= bound);	
		return retVal;		
	}

	void learnClauseProcedure(CTerm* currTerm, dlevel currAssignmentlevel);
	
};



#endif