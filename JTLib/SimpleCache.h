#ifndef SIMPLE_CACHE
#define SIMPLE_CACHE

#include "Definitions.h"
#include "Assignment.h"
#include <vector>
#include <boost/functional/hash.hpp>
using namespace boost;

typedef boost::unordered_map<size_t,probType> size_tToProbMap; //from the assignment to the cache

class DBS_equal_to
: std::binary_function<const DBS&, const DBS&, bool>{
public:
	bool operator()(const DBS& bsA, const DBS& bsB) const;
};

class DBS_hash
: std::unary_function<const DBS&, std::size_t>{
public:
	std::size_t operator()(const DBS& bitset) const;
};
typedef boost::unordered_map<DBS,probType,DBS_hash,DBS_equal_to> DBSToProb;
typedef boost::unordered_map<DBS,DBSToProb,DBS_hash,DBS_equal_to> DBSmaskToCache;

class Cache{	
public:	

	virtual ~Cache(){
		theCache.clear();
	}
	
	virtual void set(probType calcedProb, const Assignment& ga, size_t cacheKey=0){	
		size_t key = (cacheKey > 0) ? cacheKey : getKey(ga);	
		theCache[key]=calcedProb;
	}

	virtual bool get(probType& calcedProb, const Assignment& ga, size_t cacheKey=0){
		size_t key = (cacheKey > 0) ? cacheKey : getKey(ga);			
		return get(calcedProb,key);		
	}

	virtual size_t getKey(const Assignment& ga, const DBS& zeroedTerms = emptyDBS)=0;
	
protected:	
	Cache(){}
	size_t getBSKey(const DBS& BS) const;

	virtual void set(probType calcedProb, size_t key){
		theCache[key]=calcedProb;
	}

	virtual bool get(probType& calcedProb, size_t key) const{
		size_tToProbMap::const_iterator it = theCache.find(key);
		if(it != theCache.end()){
			calcedProb=it->second;
			return true;
		}
		return false;
	}

	static DBS emptyDBS;
	
private:
	size_tToProbMap theCache;
	
};

class subFormulaCache:public Cache{
public:
	subFormulaCache(const DBS& _subTerms,const DBS& _subVars):subTerms(_subTerms),subVars(_subVars){
		subTermsKeyCalc = subTerms;
		subVarsKeyCalc = subVars;
	}
	size_t getKey(const Assignment& ga, const DBS& zeroedTerms = emptyDBS);	
private:
	const DBS& subTerms;
	const DBS& subVars;
	DBS subTermsKeyCalc;
	DBS subVarsKeyCalc;

};

class assignmentCache:public Cache{
public:
	assignmentCache(const DBS& relevantVars):Cache(),varsBS(relevantVars),relevantAssignedVars(relevantVars){}
	size_t getKey(const Assignment& ga, const DBS& zeroedTerms = emptyDBS) ;
	virtual void set(probType calcedProb, const Assignment& ga, size_t cacheKey=0);
	virtual bool get(probType& calcedProb, const Assignment& ga, size_t cacheKey=0);	

private:
	const DBS& varsBS;
	DBS relevantAssignedVars; //used for key calculation	
	DBSmaskToCache maskToCache;

};



class SimpleCache: public Cache{
	typedef boost::unordered_map<size_t,probType> longToProbMap; //from the assignment to the cache
	
	typedef boost::unordered_map<size_t,longToProbMap> maskToCache;

public:
	SimpleCache(const DBS& vars);
	void init(const DBS& vars);
	void remove(const Assignment& ga);

	virtual size_t getKey(const Assignment& ga, const DBS& zeroedTerms = emptyDBS){
		return 0;
	}
	
	virtual void set(probType calcedProb, const Assignment& ga,size_t cacheKey=0);
	virtual bool get(probType& calcedProb, const Assignment& ga, size_t cacheKey=0);

	virtual ~SimpleCache(){
		subFormCache.clear();
		
		varBS.reset();
		maskMap.clear();
		maskToCache::iterator end = maskMap.end();
		for(maskToCache::iterator it = maskMap.begin() ; it != end ; ++it){
			longToProbMap& lTp = it->second;
			lTp.clear();
		}
		maskMap.clear();		
		
	}

	static size_t numOfEntries;
	
private:	
	bool get(size_t cacheKey, probType& p) const{
		size_tToProbMap::const_iterator it = subFormCache.find(cacheKey);
		if(it != subFormCache.end()){
			p=it->second;
			return true;
		}
		return false;
	}

	void set(size_t cacheKey, probType p){
		subFormCache[cacheKey]=p;
	}
	void insert(probType calcedProb, const Assignment& ga);
	bool get(const Assignment& ga, probType& p);
	

	size_t getKey(const DBS& BS);
	void clearStructures();
	//searches for var in the varArray sorted array
	int numOfVars;
	size_t localVarsSize;
	DBS varBS;
	maskToCache maskMap;	
	varType* varArr;

	//new
	size_tToProbMap subFormCache;
	size_tToProbMap assignmentCache;
};





#endif
