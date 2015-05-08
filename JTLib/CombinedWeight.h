
#ifndef COMBINED_WEIGHT_H
#define COMBINED_WEIGHT_H

#include "Params.h"
#include "Definitions.h"

class CombinedWeight{
public:
	static probType getWorstVal(bool ADD);
	CombinedWeight(){
		ADD = Params::instance().ADD_LIT_WEIGHTS;
		val = (ADD ? 0.0 : 1.0);		
	}

	CombinedWeight(const CombinedWeight& other){
		ADD = Params::instance().ADD_LIT_WEIGHTS;
		val = other.getVal();	
	}

	CombinedWeight(probType prob){
		ADD = Params::instance().ADD_LIT_WEIGHTS;
		val = prob;
	}

	void reset(){
		val = (ADD ? 0.0 : 1.0);		
	}

	probType getVal() const{
		return val;
	}

	void setVal(probType val){
		this->val = val;
	}

	inline CombinedWeight operator*(const CombinedWeight& other) const{
		CombinedWeight ret(*this);
		ret*=other;
		return ret;		
	}

	inline CombinedWeight operator*(probType otherVal) const {
		CombinedWeight ret(*this);
		ret*=otherVal;
		return ret;		
	}

	inline CombinedWeight& operator*=(const CombinedWeight& other){
		return this->operator*=(other.getVal());		
	}

	inline CombinedWeight& operator*=(probType otherVal){

		if(ADD){
			val += otherVal;
		}
		else{
			val*=otherVal;
		}		
		return *this;
	}

	inline CombinedWeight operator/(const CombinedWeight& other) const{
		CombinedWeight ret(*this);
		ret/=other;
		return ret;
	}

	inline CombinedWeight operator/(probType otherVal) const{
		CombinedWeight ret(*this);
		ret/=otherVal;
		return ret;
	}

	inline CombinedWeight& operator/=(const CombinedWeight& other){
		return this->operator/=(other.getVal());		
	}

	inline CombinedWeight& operator/=(probType otherVal){
		if(ADD){
			val -= otherVal;
		}
		else{
			val/=otherVal;
		}	
		return *this;
	}

	static probType baseVal();
	inline bool betterThanBound(probType bound) const{
		return (ADD ? val < bound : val > bound);				
	}

	static probType getBestVal(bool ADD);

private:
	probType val;
	bool ADD;
};








#endif