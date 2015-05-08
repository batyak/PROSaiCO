#include "FormulaMgr.h"
#include "Assignment.h"
#include "Utils.h"
#include <sstream>
#include <fstream>
#include "Convert.h"
#include <boost/algorithm/string.hpp>
#include "Params.h"

FormulaMgr* FormulaMgr::instance;
nodeId FormulaMgr::addTerm(const varSet& dnfVars){
	//resize the vector
	if(currTermIdx == terms.capacity()){
		terms.reserve(currTermIdx + 1000);
	}	
	CTerm term;
	term.init(dnfVars);
	termToIdxMap::const_iterator termIt = termsMap.find(term);
	if(termIt != termsMap.end()){ //term already exists
	//	return termIt->second; //in order to work in c2d settings
	}
	nodeId termIdx = currTermIdx++;
	terms.push_back(term);	
	termsMap[term]=termIdx;	
	terms[termIdx].setId(termIdx);	
//	mapTermToWatchedLits(termIdx);
	return termIdx;
}

probType FormulaMgr::preprocessBCP(bool& isSAT){
	CombinedWeight retval;
	isSAT = false; //default
	std::queue<varType> unitLits;
	varToNodeIdSet litToTerms;	
	std::vector<nodeId> markedForDeletion;
	BCPassignedVars.resize(this->numVars+1,false);
	BCPassignment.resize(this->numVars+1,false);
	for(size_t i=0 ; i < terms.size() ; i++){
		const std::vector<varType>& lits = terms[i].getLits();
		if(lits.size() == 1){
			unitLits.push(lits[0]);
		}
		for(int j=0 ; j < lits.size() ; j++){
			litToTerms[lits[j]].insert(i);			
		}		
	}
	if(unitLits.empty()){
		return retval.getVal();
	}
	
	
	while(!unitLits.empty()){
		varType toUnset = unitLits.front(); //the literal to set to false
		unitLits.pop();
		varType var = (toUnset > 0) ? toUnset : 0-toUnset;
		if(BCPassignedVars[var]){ //this var has formerly been assigned
			bool varCurrAss = BCPassignment[var]; //current value to var
			bool varRequiredAss = (toUnset > 0) ? false: true; //the value required 
			if(varCurrAss != varRequiredAss){
				isSAT = true;
				return retval.getVal(); //conflicting requirements
			}
			continue;
		}
		else{
			BCPassignedVars[var]=true; //var is now assigned
			BCPassignment[var]= (toUnset > 0) ? false:true;
		}
		//probType unsetProb = (toUnset > 0) ? vars[toUnset].getNeg_w() : vars[0-toUnset].getPos_w();
		//retval*=unsetProb;		
		varToNodeIdSet::const_iterator unsetTermsIt = litToTerms.find(toUnset); //terms containing the unset literal (to be removed)
		if(unsetTermsIt != litToTerms.end()){
			nodeIdSet termsToRemove = unsetTermsIt->second;			
			nodeIdSet::const_iterator end = termsToRemove.end();
			for(nodeIdSet::const_iterator it = termsToRemove.begin() ; it != end ; ++it){			
				nodeId termIdx = *it; //index of term to delete
				const std::vector<varType>& lits = terms[termIdx].getLits();
				for(int i=0 ; i < lits.size() ; i++){
					if(lits[i] != toUnset){					
						litToTerms[lits[i]].erase(termIdx);						
					}
				}
				markedForDeletion.push_back(termIdx); //delete the term in question
			}
			litToTerms.erase(toUnset);
		}
		//now change the terms that contain -toUnset
		varToNodeIdSet::const_iterator setTermsIt = litToTerms.find(0-toUnset); 
		if(setTermsIt != litToTerms.end()){
			nodeIdSet termsToCrop = setTermsIt->second;
			nodeIdSet::const_iterator end = termsToCrop.end();
			for(nodeIdSet::const_iterator it = termsToCrop.begin() ; it != end ; ++it){		
				nodeId termIdx = *it; //index of term to crop
				terms[termIdx].removeLiteral(0-toUnset);
				if(terms[termIdx].getNumLits() == 1){
					unitLits.push(terms[termIdx].getLits()[0]);
				}
				if(terms[termIdx].getNumLits() == 0){ //empty term means that all of its literals must be assigned --> 0 probability.
					isSAT = true;
					return retval.getVal(); //conflicting requirements
				}
			}

		}
	}	
	std::sort(markedForDeletion.begin(), markedForDeletion.end());
	for(int k = (int)markedForDeletion.size()-1 ; k >=0 ; k--){
		terms.erase(terms.begin() + markedForDeletion[k]);
	}
	terms.shrink_to_fit();
	termsMap.clear();
	for(int i=0 ; i < terms.size() ; i++){
		termsMap[terms[i]]=i;
		terms[i].setId(i);
	}
	//now calculate retVal
	for(size_t var = BCPassignedVars.find_first() ; var != boost::dynamic_bitset<>::npos ; var = BCPassignedVars.find_next(var)){
		probType p_var = (BCPassignment[var] ? vars[var].getPos_w() : vars[var].getNeg_w());
		retval*=p_var;
	}
#ifdef _DEBUG
	string BCPAssStr = Utils::printDBS(BCPassignedVars);
	cout << " BCP assignment: " << BCPAssStr << endl;
#endif
	return retval.getVal();
}

void FormulaMgr::generateLitToTermsMap(){
	for(int i=0 ; i < terms.size() ; i++){		
		const vector<varType>& lits = terms[i].getLits();
		for(int j=0 ; j < lits.size() ; j++){
			varToBS::iterator jit=litToTerms.find(lits[j]);
			if(jit == litToTerms.end()){
				DBS jLitTerms;
				jLitTerms.resize(getNumTerms());
				jLitTerms.set(i,true);
				litToTerms[lits[j]]=jLitTerms;
			}
			else{
				DBS& jLitTerms = jit->second;
				jLitTerms.set(i,true);
			}
		}
	}
	emptyTermsBS.resize(getNumTerms());
	emptyTermsBS.reset();
	emptyVarsBS.resize(numVars+1,false);
}

//return the set of terms that contain lit
const DBS& FormulaMgr::getTermsWithLit(varType lit) const{	
	varToBS::const_iterator cit = litToTerms.find(lit);
	if(cit != litToTerms.end())
		return cit->second;
	return emptyTermsBS;	
}

void FormulaMgr::setTermIds(){
	//sorts in descending term size
	//std::sort(terms.begin(), terms.end());
	for(int i=0 ; i < terms.size() ; i++){
		termsMap[terms[i]]=i;
		terms[i].setId(i);
	}
}
void FormulaMgr::generateWatchMap(){
	for(int i=0 ; i < terms.size() ; i++){
		mapTermToWatchedLits(i);
	}
}

nodeId FormulaMgr::addTerm(const vector<varType>& termVec){
	//resize the vector
	if(currTermIdx == terms.capacity()){
		terms.reserve(currTermIdx + 1000);
	}	
	CTerm term;
	term.init(termVec);
	termToIdxMap::const_iterator termIt = termsMap.find(term);
	if(termIt != termsMap.end()){ //term already exists
		return termIt->second;
	}
	nodeId termIdx = currTermIdx++;
	terms.push_back(term);	
	termsMap[term]=termIdx;
	terms[termIdx].setId(termIdx);
	mapTermToWatchedLits(termIdx);
	return termIdx;
}

void FormulaMgr::mapTermToWatchedLits(nodeId tId){
	CTerm& term = terms[tId];
	const watchedInfo* wi = term.getWatchedLiterals();
	watchedLitToDNFTermIds[wi[0].watchedLit].insert(tId);	
	watchedLitToDNFTermIds[wi[1].watchedLit].insert(tId);	
	watchedLitToDNFTermPtrs[wi[0].watchedLit].insert(&term);
	watchedLitToDNFTermPtrs[wi[1].watchedLit].insert(&term);
}



nodeId FormulaMgr::addTerm(const Assignment& Ass, const DBS& relevantVars){
	DBS newTermVarsBS(relevantVars);	
	newTermVarsBS.operator&=(Ass.getAssignedVars()); //vars relevant for the failure
	const DBS& assignment = Ass.getAssignment();
	vector<varType> newDNFTerm;
	newDNFTerm.reserve(newTermVarsBS.count());
	for(size_t i=newTermVarsBS.find_first() ; i != boost::dynamic_bitset<>::npos ; i = newTermVarsBS.find_next(i)){
		varType v = (varType)((assignment[i]) ? i : (0-i));
		newDNFTerm.push_back(v);		
	}
	return addTerm(newDNFTerm);	
}
bool FormulaMgr::updateWatchedIdx(CTerm* term, int idx,const Assignment& A){	
	const watchedInfo* termWI = term->getWatchedLiterals(); //the literals watched in term
	varType prevWatchedLit = termWI[idx].watchedLit; //literal previously watched (which we want to change).
	//DEBUG
	//Status prevLitStatus = A.getStatus(prevWatchedLit);
	//assert(prevLitStatus==SET_T);

	int otherWatchedIdx = termWI[(1-idx)].watchedIdx; //the other watched idx (idx \in {0,1})
	const std::vector<varType>&  literals = term->getLits();
	for(size_t i=0 ; i < literals.size() ; i++){ //iterate over all term literals
		if(i != otherWatchedIdx){ //i points to a non-watched lit index
			varType lit = literals[i];
			if(A.getStatus(lit) != SET_T){ 				
				term->setWatched((int)i,idx); //can be set to a new watched literal
				//update watch lists				
				watchedLitToDNFTermPtrs[lit].insert(term); 
			//	assert(insertRes.second); //make sure that the term was not previously watched by this literal
				termPtrSet& prevPtrSet = watchedLitToDNFTermPtrs.at(prevWatchedLit);
				size_t numErased = prevPtrSet.erase(term);
				assert(numErased==1); //must have been erased				
				return true;
			}			
		}
	}
	return false; //could not find other literal to watch in this term
}

void FormulaMgr::readOrderFile(string orderFile){
	std::ifstream file;
	file.open(orderFile.c_str());
	if (!file){
		cout << "CFG: File " + orderFile + " couldn't be found!\n" << endl;
		return;
	}
	std::string line;
	std::getline(file, line);
	readOrderString(line);    
}

void FormulaMgr::readOrderString(string elimOrderStr){
	vector<string> tokens; // Create vector to hold our words	
	boost::split(tokens,elimOrderStr,boost::is_any_of(" \t"),boost::token_compress_on);
	elimOrder.resize(tokens.size());
	int numVars=0;
	for(int i=0 ; i < tokens.size() ; i++){
		if(!tokens[i].empty()){
			elimOrder[i] = Convert::string_to_T<int>(tokens[i]);	
			numVars++;
		}
	}
	elimOrder.resize(numVars);
}

#define LMAP_DELIM "$"
void FormulaMgr::readLitWeightsFromLmapFile(string lmapFile){
	std::ifstream file;
	file.open(lmapFile.c_str());
	if (!file){
		cout << "CFG: File " + lmapFile + " couldn't be found!\n" << endl;
		return;
	}
	std::string line;
	size_t lineNo = 0;
	size_t numLits;
	while (std::getline(file, line)){
		lineNo++;
		std::string temp = line;

		if (temp.empty())
			continue;
		if(temp.substr(0, 3) != "cc$")
			continue; //comment	
		
		if (onlyWhitespace(temp))
			continue;

		stringstream ss(line); // Insert the string into a stream
		vector<string> tokens; // Create vector to hold our words	
		boost::split(tokens,line,boost::is_any_of(LMAP_DELIM),boost::token_compress_on);
		string type = tokens[1];
		
		if(type == "N"){
			size_t n=Convert::string_to_T<size_t>(tokens[2]);					
			numLits = n * 2;
			continue;
		}

		if(type == "V" || type == "T")
			continue;

		  // This is not a header line, a variable line, or potential line, so
		  // it must be a literal description, which looks like one of the
		  // following:
		  //   "cc" "I" literal weight srcVarName srcValName srcVal
		  //   "cc" "P" literal weight srcPotName pos+
		  //   "cc" "A" literal weight
		varType lit = Convert::string_to_T<varType>(tokens[2]);	
		probType litweight = Convert::string_to_T<probType>(tokens[3]);			
		this->updateLit(lit,litweight);		
	}
}

void FormulaMgr::getProbs(probType fileprob, probType& posProb, probType& negProb){
	if(!Params::instance().ADD_LIT_WEIGHTS){
		posProb = (fileprob < 0) ? 1.0 : 1.0-fileprob;  //we are looking at DNF 
		negProb = (fileprob < 0) ? 1.0 : fileprob;
	}
	else{
		/**
		From the paper "A dynamic apprpoach to MPE and weighted MAX-SAT"
		For every origional MAX-SAT var posProb=negProb=0
		For auxilary vars representing clauses:
		we assume that each term (clause) c_i was added with a var y_i (as opposed to -y_i in the paper)
		if y_i=1 it means that the poriginal term is UN_SAT --> p(y_i)=weight(c_i)
		otherwise, y_i=0 means that the original terms is SAT --> p(-y_i)=0
		*/
		posProb=fileprob;
		negProb=0;
	}
}

void FormulaMgr::getProbs(probType fileprob, probType filenegProb, probType& posProb, probType& negProb){
	if(!Params::instance().ADD_LIT_WEIGHTS){
		posProb=(filenegProb < 0) ? 1.0 : filenegProb;
		negProb =  (fileprob < 0) ? 1.0 : fileprob;
	}
	else{
		/**
		From the paper "A dynamic apprpoach to MPE and weighted MAX-SAT"
		For every origional MAX-SAT var posProb=negProb=0
		For auxilary vars representing clauses:
		we assume that each term (clause) c_i was added with a var y_i (as opposed to -y_i in the paper)
		if y_i=1 it means that the poriginal term is UN_SAT --> p(y_i)=weight(c_i)
		otherwise, y_i=0 means that the original terms is SAT --> p(-y_i)=0
		*/
		posProb=fileprob;
		negProb=0;
	}
}


void FormulaMgr::readFile(string cnfFilePath, bool BCP){
	clock_t t = clock();
	deterministicVarsApriori = 1.0;	
	std::ifstream file;
	file.open(cnfFilePath.c_str());
	if (!file){
		cout << "CFG: File " + cnfFilePath + " couldn't be found!\n" << endl;
		return;
	}

	std::string line;
	size_t lineNo = 0;
	while (std::getline(file, line)){
		lineNo++;
		std::string temp = line;

		if (temp.empty()|| temp.compare("c")==0)
			continue;
		if(temp[0]=='c')
			continue;
		removeComment(temp);

		if (onlyWhitespace(temp))
			continue;

		parseLine(temp, lineNo);
	}
	file.close();	
	long timeToReadFile = clock()-t;
	cout << " Time to read file: " << timeToReadFile/1000 << " seconds" << endl;
	bool isSAT;
	if(BCP){
		deterministicVarsApriori = preprocessBCP(isSAT);	
	}
	//to make sure that the term ids are correctly associated with their index in the vector
//	setTermIds();
//	generateWatchMap();		
//	generateLitToTermsMap();
}

void FormulaMgr::parseLine(const std::string &line, size_t const lineNo)
{
	string buf; // Have a buffer string
    stringstream ss(line); // Insert the string into a stream
    vector<string> tokens; // Create vector to hold our words	
	boost::split(tokens,line,boost::is_any_of(" \t"),boost::token_compress_on);
	
	if(tokens.at(0).compare("p") == 0){
		int cnfFile_numOfVars=Convert::string_to_T<int>(tokens[2]);		
		cout << " Number of vars: " << cnfFile_numOfVars << endl;
		int cnfFile_numOfTerms = Convert::string_to_T<int>(tokens[3]);
		cout << " Number of terms: " << cnfFile_numOfTerms << endl;		
		largestVar = cnfFile_numOfVars;
		init(cnfFile_numOfVars,cnfFile_numOfTerms);
		return;
	}
    
	if(tokens.at(0).compare("w") == 0){
		varType v = Convert::string_to_T<varType>(tokens[1]);
		largestVar = (v > largestVar) ? v : largestVar;
		probType v_weight=Convert::string_to_T<probType>(tokens[2]);
		probType v_pos = (v_weight < 0) ? 1.0 : 1.0-v_weight;  //we are looking at DNF 
		probType v_neg = (v_weight < 0) ? 1.0 : v_weight;
		updateVar(v,v_pos,v_neg);						
	}
	else{ //must be a CNF term
		varSet dnfTerm;
		for (vector<string>::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter){
			if((*token_iter).empty() || onlyWhitespace(*token_iter)) 
				continue;

			varType v = Convert::string_to_T<varType>(*token_iter);	
			if(v != 0){
				dnfTerm.insert(v);
			}
		}			
		if(!dnfTerm.empty()){
				//the dnf term node containing the literals				
			addTerm(dnfTerm);
		}		
	}			
		
}
void FormulaMgr::readPMAPFile(){
	string pmapFile=Params::instance().pmapFile;
	if(pmapFile.empty())
		return;
	std::ifstream file;
	file.open(pmapFile.c_str());
	if (!file){
		cout << "CFG: File " + pmapFile + " couldn't be found!\n" << endl;
		return;
	}
	std::string line;
	std::getline(file, line);

    stringstream ss(line); // Insert the string into a stream
    vector<string> tokens; // Create vector to hold our words
	boost::split(tokens,line,boost::is_any_of(" \t"),boost::token_compress_on);
	size_t numOfDets = Convert::string_to_T<size_t>(tokens[0]);
	for(size_t i = 1; i <= numOfDets ; i++){
		varType v = Convert::string_to_T<varType>(tokens[i]);
		FormulaMgr::getInstance()->setDeterminedVar(v);
	}
	cout << " There are " << FormulaMgr::getInstance()->getDeterminedVars().count() << " determined vars " << endl;
	file.close();
}

bool FormulaMgr::DBG_testZeroedClausesCorrectness(const Assignment& assignment, const DBS& zeroedClauses){	
	//check zeroed clauses
	for(size_t clauseIdx = zeroedClauses.find_first(); clauseIdx != boost::dynamic_bitset<>::npos; clauseIdx = zeroedClauses.find_next(clauseIdx)){
		CTerm& t = getTerm(clauseIdx);
		if(!t.zeroedByAssignment(assignment)){
			cout << " clause not zeroed by assignment!!" << endl;
			return false;
		}
	}
	for(size_t varIdx= assignment.getAssignedVars().find_first() ; varIdx != boost::dynamic_bitset<>::npos ; 
		varIdx = assignment.getAssignedVars().find_next(varIdx)){
			varType lit = (varType)(assignment.getAssignment()[varIdx] ? -varIdx : varIdx);
			const DBS& litTerms = getTermsWithLit(lit);					
			if(!litTerms.is_subset_of(zeroedClauses)){
				cout << " zeroed clauses does not include clauses containing " << lit << endl;
				return false;
			}
	}
	return true;
}
/*
bool FormulaMgr::satisfyDNFNode(nodeId id, const Assignment& ass, Assignment& implied){	
	CTerm& t = terms[id];
	return t.isSatisfied(ass,implied);	
}

bool FormulaMgr::satisfyAnyDNFNode(const nodeIdSet& ids, const Assignment& ass, Assignment& implied){
	nodeIdSet::const_iterator end = ids.end();
	for(nodeIdSet::const_iterator it = ids.begin() ; it != end ; ++it){
		if(satisfyDNFNode(*it,ass,implied)){
			return true;
		}
	}
	return false;
}

bool FormulaMgr::satisfyAnyDNFNode(const vector<nodeId>& ids, const Assignment& ass,Assignment& implied ){
	vector<nodeId>::const_iterator end = ids.end();
	for(vector<nodeId>::const_iterator it = ids.begin() ; it != end ; ++it){
		if(satisfyDNFNode(*it,ass,implied)){
			return true;
		}
	}
	return false;
}
*/
