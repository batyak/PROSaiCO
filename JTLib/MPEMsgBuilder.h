#ifndef MPE_MSGBUILDER_H
#define MPE_MSGBUILDER_H


#include "MsgGenerator.h"
#include "SimpleCache.h"

using namespace boost;
class MPEAssignmentCache;
class MPECacheStorage;
class MPESubFormCache;
class MPECache;

class MPEMsgBuilder: public MSGGenerator{
public:
	
	MPEMsgBuilder(TreeNode* TN);
	virtual probType buildMsg(Assignment& context,probType lb, DBS& zeroedClauses,Assignment* bestAssignment=NULL);	
	//virtual probType buildMsg_2(Assignment& context,probType lb, DBS& zeroedClauses);	
	void initByTN();	
	void updateBestAssignment(Assignment& context, DBS& zeroedClauses, Assignment& MPEAss);	
	MPECache* getCache() const{
		return Cache;
	}

	MPECache* getUBCache() const{
		return upperBoundCache;
	}

	virtual void clearCacheSaveUP();
	

protected:
	virtual probType getDNFTermMPProb(const Assignment& context, Assignment* MPEAss = NULL) const;
	virtual probType OptimalAssignmentProb(const DBS& unassignedVars,Assignment* optimalAss = NULL ) const;	
	virtual probType childrenUpperBounds(const Assignment& context);
	/*
	When ignoreParent=true then we set the variable DESPITE the fact that it is unassiged at the parent node.
	The reason for this is that we may currently be in the branch that origionally contained UNSAT terms.
	Following a configuration (currAss) the temrs containing the var have becom SAT.
	If these vars are not set at this point, they will never be set. The parent expects the current brnch to assign it
	and the rest of the brnaches are not allowed to set it.
	*/
	virtual void setUnaffectingVarsMPE(const DBS& zeroedClauses, const DBS& vars, Assignment& MPEAssignment);	
	virtual ~MPEMsgBuilder(){}

	


private:	
	MPECache* Cache; //TODO: cache should also contain the appropriate assignment to the variables!!!	
	MPECache* upperBoundCache;		
	bool ADD_VARS;
	//for computing optimal assignments
	DBS assignedChildVars;
	DBS unassignedChildVars;
	//returns true if p1 is preferred over p2
	virtual inline bool compareLitProbs(probType p1, probType p2) const{
		return (p1 >= p2);
	}

	//returns true if a1 is preferred over a2
	virtual inline bool compareAssignmentProbs(probType a1, probType a2) const{
		return (a1 > a2);
	}

	virtual inline probType worstZeroOneRatio() const{
		return 0.0; //under the assumption that we want the largest ratio 
	}
	
	virtual inline probType computeRatio(probType negProb, probType posProb) const{
		return (negProb/posProb); //in ADD case, should be negProb - posProb
	}

	virtual inline probType worstRetVal(probType bound) const{
		return (bound/1.001); //in MULT case return a result that is a bit worse than the bound		
	}


	virtual inline bool compareRatios(probType r1, probType r2) const{
		return (r1 > r2);
	}
	
	probType getChildGenericBound(TreeNode* child) const;
	virtual void calculategenericUpperBounds(TreeNode* node);

	inline probType bestProbForVar(varType v) const{
		v = (v > 0) ? v :-v;
		probType posProb = FM.getLitProb(v);
		probType negProb = FM.getLitProb(-v);
		if(compareLitProbs(posProb,negProb)){
			return posProb;
		}
		return negProb;
	}
};


class MPECacheStorage{
public:
	MPECacheStorage(){
		p=1.0;
	}
	MPECacheStorage(probType p_, Assignment* MPEAss_, DirectedGraph* DOG):p(p_),MPEAss(MPEAss_),DOG(DOG){}
	probType p;
	Assignment* MPEAss;
	DirectedGraph* DOG;
};

typedef boost::unordered_map<DBS,MPECacheStorage*,DBS_hash,DBS_equal_to> DBSToMPEStore;
typedef boost::unordered_map<DBS,DBSToMPEStore,DBS_hash,DBS_equal_to> DBSmaskToMPEStore;



class MPEAssignmentCache:public Cache{
public:
	MPEAssignmentCache(const DBS& relevantVars):Cache(),varsBS(relevantVars),relevantAssignedVars(relevantVars){}
	
	size_t getKey(const Assignment& ga, const DBS& zeroedTerms = emptyDBS) {
		return 0;
	}
	virtual void set(MPECacheStorage* MPEResult, const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0);
	virtual MPECacheStorage* get(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0);

	
private:
	const DBS& varsBS;
	DBS relevantAssignedVars; //used for key calculation	
	DBSmaskToMPEStore maskToCache;
	
};


class MPESubFormCache:public subFormulaCache{
	typedef boost::unordered_map<size_t,MPECacheStorage*> size_tToMPEStore; 
public:
	MPESubFormCache(const DBS& _subTerms,const DBS& _subVars):subFormulaCache(_subTerms,_subVars){}
	virtual void set(MPECacheStorage* MPEResult, const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
#ifdef _DEBUG
		assert(MPEResult == NULL || MPEResult->MPEAss == NULL ||
			(ga.getAssignedVars().intersects(MPEResult->MPEAss->getAssignedVars())==false));
#endif
		size_t key = (cacheKey > 0) ? cacheKey : getKey(ga,zeroedTerms);
		subFormCache[key]=MPEResult;		
	}

	virtual MPECacheStorage* get(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
		size_t key = (cacheKey > 0) ? cacheKey : getKey(ga,zeroedTerms);
		size_tToMPEStore::const_iterator it = subFormCache.find(key);
		if(it != subFormCache.end()){
			return it->second;			
		}
		return NULL;
	}

	void deleteEntry(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
		size_t key = (cacheKey > 0) ? cacheKey : getKey(ga,zeroedTerms);
		size_tToMPEStore::const_iterator it = subFormCache.find(key);
		if(it != subFormCache.end()){
			MPECacheStorage* MPEResult = it->second;
			subFormCache.erase(it);	
			delete MPEResult;
		}		
	}

	virtual size_t numEntries() const{
		return subFormCache.size();
	}


	void addAll(MPESubFormCache* other){
		subFormCache.insert(other->subFormCache.begin(), other->subFormCache.end());
	}

	void clearAll(){
		subFormCache.clear();
	}
private:
	size_tToMPEStore subFormCache;
	
};

class MPECache{
public:
	MPECache(TreeNode* TN){
		assignmentCache = NULL;
		subFormCache = NULL;
		if(Params::instance().DYNAMIC_SF){
			subFormCache = new MPESubFormCache(TN->getDNFIds(),TN->getSubtreeVars());			
		}
		else{
			assignmentCache = new MPEAssignmentCache(TN->getSubtreeVars());					
		}
	}

	virtual void set(MPECacheStorage* MPEResult, const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
		if(assignmentCache != NULL){
			assignmentCache->set(MPEResult,ga,zeroedTerms,cacheKey);
		}
		else{
			subFormCache->set(MPEResult,ga,zeroedTerms,cacheKey);;
		}
	}

	virtual MPECacheStorage* get(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
		if(assignmentCache != NULL){
			return assignmentCache->get(ga,zeroedTerms,cacheKey);
		}
		else{
			return subFormCache->get(ga,zeroedTerms,cacheKey);;
		}
	}

	size_t getKey(const Assignment& ga, const DBS& zeroedTerms){
		if(assignmentCache != NULL){
			return assignmentCache->getKey(ga,zeroedTerms);
		}
		else{
			return subFormCache->getKey(ga,zeroedTerms);
		}
	}

	void deleteEntry(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey=0){
		if(subFormCache != NULL){
			subFormCache->deleteEntry(ga,zeroedTerms,cacheKey);
		}
	}

	size_t numEntries() const{
		return subFormCache->numEntries();
	}

	virtual ~MPECache(){
		if(assignmentCache!= NULL){
			delete assignmentCache;
		}
		if(subFormCache != NULL){
			delete subFormCache;
		}
	}

	void addAll(MPECache* other){
		if(subFormCache != NULL){
			subFormCache->addAll(other->subFormCache);
		}		
	}

	void clearAll(){
		if(subFormCache != NULL){
			subFormCache->clearAll();
		}
		
	}

private:
	MPEAssignmentCache* assignmentCache;
	MPESubFormCache* subFormCache;
};



#endif
