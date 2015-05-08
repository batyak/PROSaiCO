#ifndef CVAR_H
#define CVAR_H

#include "Definitions.h"
#include "CTerm.h"
#include <list>

class CVariable{	
public:
	CVariable(){
		pos_w = 0.5;
		neg_w = 0.5;	
		antecendent = NULL;
		assLevel=0;
		v=0;
		nodeLevel=0;
	}

	CVariable(varType v_, probType pos_w_, probType neg_w_){
		updateVar(v_,pos_w_,neg_w_);
		antecendent = NULL;
	}

	void updateVar(varType v_, probType pos_w_, probType neg_w_){
		v = v_;
		pos_w = pos_w_;
		neg_w = neg_w_;				
	}	

	probType getPos_w() const{
		return pos_w;
	}

	probType getNeg_w() const{
		return neg_w;
	}

	void setPos_w(probType pos) {
		pos_w = pos;
	}

	void setNeg_w(probType neg) {
		neg_w=neg;
	}

	varType getV() const{
		return v;
	}

	void setAntecendent(CTerm* ant){
		this->antecendent = ant;
	}

	CTerm* getAntecendent(){
		return antecendent;
	}

	void setDLevel(dlevel nodeLevel, dlevel assLevel){
		this->nodeLevel = nodeLevel;
		this->assLevel = assLevel;
	}	

	dlevel getAssignmentLevel(){
		return assLevel;
	}

	dlevel getNodeLevel(){
		return nodeLevel;
	}	

private:	
	varType v;
	probType pos_w;
	probType neg_w;	
	//CDCL
	CTerm* antecendent;

	dlevel assLevel;
	dlevel nodeLevel;
	
};



#endif
