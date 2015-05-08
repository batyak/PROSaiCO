
#ifndef ASSIGNMENT_H
#define ASSIGNMENT_H

#include <string>
#include "boost/dynamic_bitset.hpp"
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include "Definitions.h"
#include "FormulaMgr.h"
#include "Utils.h"
#include "CombinedWeight.h"
using namespace std;

 class Assignment{

public:

	Assignment();
	Assignment(const Assignment& other);
	Assignment(const DBS& relevantVars, const DBS& varsAssignment);
	Assignment& operator= (const Assignment & other);
		
	virtual void clearAssignment(){
		assignedVars.reset();
		assignment.reset();		
		assProb.reset();
	}
	
	virtual void setVar(varType var, bool val){		
		varType v = (var > 0) ? var : 0-var;
		bool val_ = (var > 0) ? val : !val;
		CombinedWeight multBy;
		CombinedWeight divBy;
		probType vProb = FM.getVar(v).getPos_w(); //varToProb.at(v);
		probType negVProb = FM.getVar(v).getNeg_w(); //varToProb.at(0-v);
		if(!assignedVars[v]){ //was not previously assigned
			multBy.setVal((val_) ? vProb : negVProb);
		}
		else{ //v already has a value
			if(val_ != assignment[v]){ //value has changed
				divBy.setVal((assignment[v]) ? vProb : negVProb);
				multBy.setVal((val_) ? vProb : negVProb);
			}
		}
		assignedVars[v]=true;
		assignment[v] = val_;	
		assProb*=multBy;
		assProb/=divBy;		
		
	}

	virtual void unsetVar(varType var){		
		varType v = (var > 0) ? var : 0-var;				
		CombinedWeight divBy;
		probType vProb = FM.getVar(v).getPos_w(); //varToProb.at(v);
		probType negVProb = FM.getVar(v).getNeg_w(); //varToProb.at(0-v);
		if(!assignedVars[v]){ //was not previously assigned
			return;
		}
		else{ //v already has a value
			divBy.setVal((assignment[v]) ? vProb : negVProb);
		}
		assignedVars[v]=false;
		assignment[v] = false;	
		assProb/=divBy;		
	}

	Status getStatus(varType var) const{
		varType v = (var > 0) ? var : 0-var;
		if(!assignedVars[v]){
			return UNSET;
		}
		bool vAss = assignment[v];
		bool retVal = (var > 0) ? vAss : !vAss; //var's assignment
		return (retVal ? SET_T : SET_F);
	}

	//assumption: other.assignedVars \cap this.assignedVars = \emptyset
	virtual void setOtherAssignment(const Assignment& other){		
		#ifdef _DEBUG
			bool intersects = 	other.assignedVars.intersects(assignedVars);				
			assert(!intersects);
		#endif
		assignedVars.operator|=(other.assignedVars);		
		assignment.operator|=(other.assignment);
		//assProb*=other.assProb;
		assProb*=other.assProb;		
	}

	//assumption: calling this function means that setting varToSet to false
	//doe not affect the assignment prob
	virtual void setAllToFalse(const DBS& varsToSet){
		#ifdef _DEBUG
			bool intersects = 	varsToSet.intersects(assignedVars);				
			assert(!intersects);
		#endif
		assignedVars.operator|=(varsToSet);
	}

	//assumption: other.assignedVars \cap this.assignedVars = \emptyset
	virtual void clearOtherAssignment(const Assignment& other){		
		#ifdef _DEBUG
			bool subset = other.assignedVars.is_subset_of(assignedVars);
			assert(subset);
		#endif
		assignedVars.operator-=(other.assignedVars);
		assignment.operator&=(assignedVars);
		assProb/=other.assProb;
	}

	//returns true if all assigned vars are set to 1
	bool allTrue() const;

	string print() const;

	const CombinedWeight& getAssignmentProb() const{
		return assProb;
	}
	
	const DBS& getAssignedVars() const{
		return assignedVars;
	}

	const DBS& getAssignment() const{
		return assignment;
	}

	int getNumVars() const{
		return numOfVars+1;
	}

	virtual ~Assignment(){		
		assignedVars.reset();
		assignment.reset();
		assProb.reset();
	}

	//return true if the messages are equal, false otherwise
    bool operator == (const Assignment &other) const{
		return (other.getAssignedVars()==assignedVars && 
				other.getAssignment() == assignment);
    }

private:

	DBS assignedVars;
	DBS assignment;
	//probType assProb;	
	CombinedWeight assProb;
	//const varToProbMap& varToProb;
	FormulaMgr& FM;
	int numOfVars;


};


#endif
