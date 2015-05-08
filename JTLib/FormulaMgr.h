#ifndef FORMULA_MGR_H
#define FORMULA_MGR_H

#include "Definitions.h"
#include "CTerm.h"
#include "CVariable.h"
#include <boost/unordered_map.hpp>
#include "Params.h"
#define COMMENT_CHAR "c "

using namespace std;
using namespace boost;

class FormulaMgr{
public:
	
	FormulaMgr(){
		currTermIdx = 0;
		largestVar = 0;		
		numOfOrigTerms=0;
		numVars=0;
		deterministicVarsApriori=1.0;
		FormulaMgr::instance = this;
		allVarProbsLessThan1=true;
	}

	FormulaMgr(string cnfFile, bool BCP = true){
		currTermIdx = 0;
		largestVar = 0;		
		readFile(cnfFile,BCP);
		//FormulaMgr::instance = this;
	}

	void init(int numOfVars, int numOfTerms){
		numVars = numOfVars;		
		largestVar = 0;
		numOfOrigTerms = numOfTerms;
		vars.reserve(numOfVars+1);		
		terms.reserve(numOfTerms+1);		
		currTermIdx = 0;
		addVar(0,0.5,0.5);
		for(varType i = 1; i <= numOfVars ; ++i){
			addVar(i,0.5,0.5);
		}		
		MPVars.resize(this->numVars+1,false);
		determinedVars.resize(this->numVars+1,false);
		weight1Vars.resize(this->numVars+1,false);
	}

	void addVar(varType v, probType pos_w, probType neg_w){
		CVariable CV(v,pos_w,neg_w);
		if(v >= vars.capacity()){
			vars.reserve(v + 1000);
		}
		vars.push_back(CV);
		largestVar = (v > largestVar) ? v : largestVar;
		//watchedLitToDNFTerms[v].resize(getNumTerms());
		//watchedLitToDNFTerms[0-v].resize(getNumTerms());
	}

	void updateVar(varType v, probType pos_w, probType neg_w){
		vars[v].updateVar(v,pos_w,neg_w);	
		if(Params::instance().ADD_LIT_WEIGHTS){
			if(pos_w==0.0 && neg_w==0.0){
				weight1Vars[v]=true;
			}
		}
		else{
			if(pos_w==1.0 && neg_w==1.0)
				weight1Vars[v]=true;
		}		
		if(pos_w> 1.0 || neg_w > 1.0){
			allVarProbsLessThan1=false;
		}
	}

	bool get_allVarProbsLessThan1() const{
		return allVarProbsLessThan1;
	}

	void updateLit(varType lit, probType litprob){
		varType v = (lit < 0) ? 0-lit : lit;
		if(lit < 0){ //this is the probability that v = 0 --> i.e., since DNF - this is the positive prob
			vars[v].setPos_w(litprob);
		}
		else{
			vars[v].setNeg_w(litprob);
		}
	}

	nodeId addTerm(const varSet& term);
	nodeId addTerm(const vector<varType>& term);

	/*
		In this scenario, failingAssignment caused the formula to evaluate to true (false)
		where the relevant vars were those in relevantVars
	*/
	nodeId addTerm(const Assignment& Ass, const DBS& relevantVars);

	probType getProb(varType v) const{		
		return ((v < 0) ? vars[0-v].getNeg_w(): vars[v].getPos_w());
	}
	
	CVariable& getVar(varType v){
		v = (v > 0) ? v : 0-v;
		return vars[v];
	}

	CTerm& getTerm(nodeId id){
		return terms[id];
	}

	vector<CTerm>& getTerms(){
		return terms;
	}

	size_t getNumTerms(){
		//return (numOfOrigTerms+1);
		return (terms.size()+1);
	}

	int getNumVars() const{
		return largestVar;
	}

	const vector<CVariable>& getVars() const{
		return vars;
	}

	probType getLitProb(varType lit) {
		CVariable& var = getVar(lit);
		if(lit > 0){
			return var.getPos_w();
		}
		else{
			return var.getNeg_w();
		}
	}
	/*
	 Assumption watchedLit==1 so we want to update its location in the map according to the 
	 newly watched literals
	*/
	void updateMappedTerms(varType watchedLit){
	
		varToNodeIdSet::iterator litTermsIt = watchedLitToDNFTermIds.find(watchedLit);
		if(litTermsIt == watchedLitToDNFTermIds.end()){
			return; //already empty.
		}
		nodeIdSet& termIds = litTermsIt->second;		
		for(nodeIdSet::iterator termIt = termIds.begin() ; termIt != termIds.end();){
			nodeId termId = *termIt;
			const CTerm& term = terms[termId];
			const watchedInfo* wi = term.getWatchedLiterals();			
			watchedLitToDNFTermIds[wi[0].watchedLit].insert(termId);
			watchedLitToDNFTermIds[wi[1].watchedLit].insert(termId);
			if(wi[0].watchedLit != watchedLit && wi[1].watchedLit != watchedLit){				
				termIt = termIds.erase(termIt);
			}
			else{
				++termIt;
			}			
		}
	}
	
	const nodeIdSet& getTermIdsByWatchedLit(varType lit) const{
		varToNodeIdSet::const_iterator nit = watchedLitToDNFTermIds.find(lit);
		if(nit == watchedLitToDNFTermIds.end()){
			return emptyNIS;
		}
		return nit->second;
	}

	const termPtrSet& getTermPtrsWatchedLit(varType lit) const{
		litToTermPtrSet::const_iterator it = watchedLitToDNFTermPtrs.find(lit);
		if(it == watchedLitToDNFTermPtrs.end()){
			return emptyTPS;
		}
		return it->second;
	}

	void setAllVarsMP(){
		MPVars.set();
	}

	const DBS& getMPVars() const{
		return MPVars;
	}
	
	bool isMPVar(varType l) const{
		varType v = (l < 0) ? 0-l : l;
		return MPVars[v];	
	}

	bool isDeterminedVar(varType l) const{
		varType v = (l < 0) ? 0-l : l;
		return determinedVars[v];	
	}


	void setDeterminedVar(varType l){
		varType v = (l < 0) ? 0-l : l;
		determinedVars.set(v,true);	
	}

	void generateWatchMap();
	probType preprocessBCP(bool& isSAT);
	bool updateWatchedIdx(CTerm* term, int idx,const Assignment& A);	
	void setTermIds();

	void generateLitToTermsMap();
	const DBS& getTermsWithLit(varType lit) const;

	const DBS& getBCPAssignedVars() const {
		return BCPassignedVars;
	}

	const DBS& getBCPAssignment() const {
		return BCPassignment;
	}

	static FormulaMgr* getInstance(){
		return instance;
	}

	const DBS& getDeterminedVars() const{
		return determinedVars;
	}

	const DBS& getWeight1Vars() const{
		return weight1Vars;
	}

	void readFile(string cnfFilePath, bool BCP = true);
	const varToBS& getLitToTerms() const{
		return litToTerms;
	}
	probType getEvidenceProb() const{
		return deterministicVarsApriori;
	}

	void readOrderFile(string orderFile);
	const vector<varType>& getElimOrder() const{
		return elimOrder;
	}
	void readOrderString(string elimOrder);
	void readLitWeightsFromLmapFile(string lmapFile);
	void readPMAPFile();

	static void getProbs(probType fileprob, probType& posProb, probType& negProb);
	static void getProbs(probType fileprob, probType filenegProb, probType& posProb, probType& negProb);
	bool DBG_testZeroedClausesCorrectness(const Assignment& assignment, const DBS& zeroedClauses);
	const DBS& getEmptyTermsDBS() const{
		return emptyTermsBS;
	}
	const DBS& getEmptyVarsDBS() const{
		return emptyVarsBS;
	}
	
private:	
	void mapTermToWatchedLits(nodeId tId);
	varToNodeIdSet watchedLitToDNFTermIds;	
	litToTermPtrSet watchedLitToDNFTermPtrs;	
	//varToDNFTerms watchedLitToDNFTerms;
	termToIdxMap termsMap;
	vector<CVariable> vars;
	vector<CTerm> terms;	
	nodeId currTermIdx;
	int numVars;
	int numOfOrigTerms;
	int largestVar;
	nodeIdSet emptyNIS;
	termPtrSet emptyTPS;
	DBS emptyTermsBS;	
	DBS emptyVarsBS;	
	DBS MPVars;
	DBS determinedVars;
	DBS weight1Vars; //used for optimizing MPE executions
	bool allVarProbsLessThan1;
	varToBS litToTerms;

	DBS BCPassignedVars;
	DBS BCPassignment;
	static FormulaMgr* instance;

	probType deterministicVarsApriori;

	void removeComment(std::string &line)
	{
		if (line.find(COMMENT_CHAR) != line.npos)
		line.erase(line.find(COMMENT_CHAR));
	}

	bool onlyWhitespace(const std::string &line)
	{
		return (line.find_first_not_of(' ') == line.npos);
	}

	void parseLine(const std::string &line, size_t const lineNo);	

	vector<varType> elimOrder;
	
};












#endif