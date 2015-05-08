#ifndef LTERM_H
#define LTERM_H


#include "CTerm.h"

class LearnedTerm: public CTerm{
public:
	LearnedTerm():CTerm(){
		isLearned = true;
	}	
	void init(const varSet& dnfTerm){		
		literals.reserve(dnfTerm.size());
		varSet::const_iterator end = dnfTerm.cend();
		for(varSet::const_iterator it = dnfTerm.begin() ; it != end ; ++it){
			literals.push_back(*it);
		}
		std::sort(literals.begin(), literals.end());				
	}
	void setWatched(int litIdx, int watchInfoIdx){
		CTerm::setWatched(litIdx, watchInfoIdx);
	}

	bool equals(const LearnedTerm& LT) const{
		return (getLits()==LT.getLits());
	}
private:

};










#endif