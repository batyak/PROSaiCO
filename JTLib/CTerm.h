#ifndef CTERM_H
#define CTERM_H

#include "Definitions.h"
#include <queue>
#include <string>
#include <sstream>
class Assignment;

struct watchedInfo{
	int watchedIdx;
	varType watchedLit;
	varType watchedVar;
};


class CTerm{	
public:
	CTerm(){
		isLearned=false;
	}

	void init(const varSet& dnfTerm){		
		literals.reserve(dnfTerm.size());
		varSet::const_iterator end = dnfTerm.cend();
		for(varSet::const_iterator it = dnfTerm.begin() ; it != end ; ++it){
			literals.push_back(*it);
		}
		std::sort(literals.begin(), literals.end());		
		setWatched(0,0);//random selection
		if(literals.size() > 1){
			setWatched(1,1);		
		}
		else{
			setWatched(0,1);		
		}
	}
	
	void init(const std::vector<varType>& dnfTerm){		
		literals.reserve(dnfTerm.size());
		copy(dnfTerm.begin(), dnfTerm.end(),back_inserter(literals));		
		std::sort(literals.begin(), literals.end());		
		setWatched(0,0);//random selection
		if(literals.size() > 1){
			setWatched(1,1);		
		}
		else{
			setWatched(0,1);		
		}		
	}

	bool operator < (const CTerm& C1) const{
        return (literals.size() >  C1.literals.size());
    }

	size_t getNumLits() const{
		return literals.size();
	}	

	const std::vector<varType>& getLits() const{
		return literals;
	}
	//return true if there has been an update
	bool configureWatchedLits(const Assignment& Ass);	
	bool satisfiedByAssignment(const Assignment& GA) const;
	bool zeroedByAssignment(const Assignment& GA) const;
	bool isUnitByAssignment(const Assignment& GA, varType& unitLit);
	bool DBGIsRelTerm(const Assignment& GA) const;

	

	void getDNFVarSet(varSet& vars) const{
		for(size_t i=0 ; i < literals.size() ; i++){
			varType lit = literals[i] ;
			varType v = (lit > 0) ? lit : 0-lit;
			vars.insert(v);
		}
	}
	
	void getDNFLitSet(varSet& vars) const{
		for(size_t i=0 ; i < literals.size() ; i++){
			varType lit = literals[i] ;			
			vars.insert(lit);
		}
	}

	const watchedInfo* getWatchedLiterals() const{
		return watched;
	}

	std::string toString() const{
		std::stringstream ss;
		for(std::vector<varType>::const_iterator litIt = literals.begin() ; litIt != literals.end() ; ++litIt){
			ss <<  *litIt << " ";
		}		
		return ss.str();	
	}

	bool getIsLearned() const{
		return isLearned;
	}

	void removeLiteral(varType lit){
		std::vector<varType>::const_iterator end = literals.end();
		std::vector<varType>::iterator it;
		for(it = literals.begin() ; it != end ; ++it){
			if(*it == lit){
				break;
			}
		}
		if(it != end){
			literals.erase(it);
			literals.shrink_to_fit();
			if(literals.empty())
				return;
			setWatched(0,0);//random selection
			if(literals.size() > 1){
				setWatched(1,1);		
			}
			else{
				setWatched(0,1);		
			}
		}
	}
	void setWatched(int litIdx, int watchInfoIdx){		
		watched[watchInfoIdx].watchedIdx = litIdx;
		watched[watchInfoIdx].watchedLit = literals[litIdx];
		watched[watchInfoIdx].watchedVar = (watched[watchInfoIdx].watchedLit > 0) ? watched[watchInfoIdx].watchedLit : 0- watched[watchInfoIdx].watchedLit;							
	}

	void setId(nodeId id){
		this->termId = id;
	}

	nodeId getId() const{
		return termId;
	}

	void setWatchedLits(varType lit1, varType lit2);
	bool updateWatchedIdx(int idx,const Assignment& A);	
protected:
	std::vector<varType> literals;	
private:		
	watchedInfo watched[2];	
	nodeId termId;

	Status getWatchStatus(const watchedInfo& wf,const Assignment& ass) const;
	

	int getLitIndex(varType lit) const{		
		for(int i=0 ; i < literals.size() ; i++){
			if(literals[i] == lit){
				return i;
			}
		}
		return -1;
	}

protected:

	
	bool isLearned;
};


struct term_equal_to
: std::binary_function<const CTerm&, const CTerm&, bool>
{
	bool operator()(const CTerm& termA, const CTerm& termB) const
	{	
		return (termA.getLits()==termB.getLits());					
	}
};

struct term_hash
: std::unary_function<const CTerm&, std::size_t>
{
	std::size_t operator()(const CTerm& term) const
	{
		size_t seed = 0;
		std::vector<varType>::const_iterator theEnd = term.getLits().end();
		for(std::vector<varType>::const_iterator it = term.getLits().begin(); it != theEnd; ++it){
			boost::hash_combine(seed,*it);			
		}                

		return seed;
	}
};

typedef boost::unordered_map<CTerm,size_t,term_hash,term_equal_to> termToIdxMap;
typedef boost::unordered_map<CTerm,CTerm*,term_hash,term_equal_to> termToPtrMap;;
//typedef boost::unordered_set<CTerm,term_hash,term_equal_to> termSet;

typedef boost::unordered_set<CTerm*> termPtrSet; //a set of ptrs to CTerm
typedef std::vector<CTerm*> termPtrVec; //a vector of ptrs to CTerm
typedef boost::unordered_map<varType, termPtrSet> litToTermPtrSet; //from a literal to the terms watched by it.

#endif
