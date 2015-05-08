#include "SimpleCache.h"
#include "Definitions.h"
#include "ContractedJunctionTree.h"
#include <functional>
#include <boost/functional/hash.hpp>

using namespace boost;

size_t SimpleCache::numOfEntries = 0;
DBS Cache::emptyDBS;
size_t Cache::getBSKey(const DBS& BS) const{			
	return boost::hash_value(BS.m_bits);		
}

size_t subFormulaCache::getKey(const Assignment& ga, const DBS& zeroedTerms){
	//size_t key = 0;		
	//init
	subTermsKeyCalc.operator|=(subTerms);
	subVarsKeyCalc.operator|=(subVars);
	//subVarsKeyCalc.operator-=(FormulaMgr::getInstance()->getDeterminedVars());
	//update
	subTermsKeyCalc.operator&=(zeroedTerms);
	subVarsKeyCalc.operator&=(ga.getAssignedVars());
	//get keys
	size_t zeroedTermsKey = getBSKey(subTermsKeyCalc);
	size_t instVarsKey = getBSKey(subVarsKeyCalc);
	size_t key = zeroedTermsKey;
	//boost::hash_combine(key,zeroedTermsKey);
	boost::hash_combine(key,instVarsKey);
	return key;
	/*
	DBS zeroedSubTerms = subTerms;
	zeroedSubTerms.operator&=(zeroedTerms);
	size_t zeroedTermsKey = getBSKey(zeroedSubTerms);
	DBS instVars = subVars;
	instVars.operator&=(ga.getAssignedVars());
	size_t instVarsKey = getBSKey(instVars);

	boost::hash_combine(key,zeroedTermsKey);
	boost::hash_combine(key,instVarsKey);
	return key;*/
}

size_t assignmentCache::getKey(const Assignment& ga, const DBS& zeroedTerms) {
	return 0;
}


void assignmentCache::set(probType calcedProb, const Assignment& ga,  size_t cacheKey){
	relevantAssignedVars.operator|=(varsBS); //make sure this DBS contains the relevant vars
	relevantAssignedVars.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	DBSToProb& cache = maskToCache[relevantAssignedVars]; //get appropriate cache entry
	
	//now get the assignment key for the hash
	relevantAssignedVars.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	cache[relevantAssignedVars]= calcedProb; //place in the cache	
}

bool assignmentCache::get(probType& calcedProb, const Assignment& ga, size_t cacheKey){
	relevantAssignedVars.operator|=(varsBS); //make sure this DBS contains the relevant vars
	relevantAssignedVars.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	DBSToProb& cache = maskToCache[relevantAssignedVars]; //get appropriate cache entry

	//now get the assignment key for the hash	
	relevantAssignedVars.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	DBSToProb::const_iterator it = cache.find(relevantAssignedVars);
	if(it == cache.cend()){
		return false;
	}
	calcedProb = it->second;
	return true;
}


SimpleCache::SimpleCache(const DBS& vars){
	init(vars);
}

void SimpleCache::clearStructures(){
	numOfVars = ContractedJunctionTree::largestVar;	
	//clear up the maps	
	maskMap.clear();
//	hashVec.clear();	
	//build and clear the bitsets
	varBS.resize(numOfVars+1);	
	varBS.reset();	
	localVarsSize = 0;
}

void SimpleCache::init(const DBS& vars){
	clearStructures();
	localVarsSize = vars.count();
	//clear the vector for the hash cumputation
	varArr = new varType[localVarsSize];
	varBS=vars;	
	int ind = 0;
	for(size_t var=varBS.find_first(); var != boost::dynamic_bitset<>::npos; var = varBS.find_next(var) ){
		varArr[ind++]=(varType)var;
	}
	std::sort(varArr, varArr+localVarsSize);
}

void SimpleCache::remove(const Assignment& ga){
	DBS mask = varBS; //the relevant vars
	mask.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	std::size_t maskHash = getKey(mask); //get the hash corresponding tp to the set of assigned vars	
	longToProbMap& cache = maskMap[maskHash]; //get appropriate cache entry

	//now get the assignment key for the hash	
	mask.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	std::size_t keyHash=getKey(mask);		
	longToProbMap::const_iterator it =  cache.find(keyHash);
	if(it == cache.cend()){
		return; //not there
	}
	cache.erase(it);
}

void SimpleCache::set(probType calcedProb, const Assignment& ga, size_t cacheKey){
	insert(calcedProb,ga); 		
}



void SimpleCache::insert(probType calcedProb, const Assignment& ga){
	DBS mask = varBS; //the relevant vars
	mask.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	std::size_t maskHash = getKey(mask); //get the hash corresponding tp to the set of assigned vars	
	longToProbMap& cache = maskMap[maskHash]; //get appropriate cache entry

	//now get the assignment key for the hash
	mask.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	std::size_t keyHash=getKey(mask);		
	cache[keyHash]=calcedProb; //place in the cache
	numOfEntries++;	
}

bool SimpleCache::get(probType& calcedProb, const Assignment& ga, size_t cacheKey){
	return get(ga,calcedProb);
}

bool SimpleCache::get(const Assignment& ga, probType& p){
	DBS mask = varBS; //the relevant vars
	mask.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	std::size_t maskHash = getKey(mask); //get the hash corresponding tp to the set of assigned vars	
	longToProbMap& cache = maskMap[maskHash]; //get appropriate cache entry

	//now get the assignment key for the hash	
	mask.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	std::size_t keyHash=getKey(mask);		
	longToProbMap::const_iterator it =  cache.find(keyHash);
	if(it == cache.cend()){
		return false;
	}
	p=it->second;
	return true;
}


size_t SimpleCache::getKey(const DBS& BS){			
	return boost::hash_value(BS.m_bits);		
}

bool DBS_equal_to::operator()(const DBS& bsA, const DBS& bsB) const{	
		return (bsA==bsB);		
}

std::size_t DBS_hash::operator()(const DBS& bitset) const{
		return boost::hash_value(bitset.m_bits);		
}