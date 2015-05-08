#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <time.h>
#include "Assignment.h"
#include "ContractedJunctionTree.h"

Assignment& Assignment::operator=(const Assignment & other){
	if(this == &other){
		return *this;
	}
	FM = other.FM;
	numOfVars = other.numOfVars;
	assignedVars=other.assignedVars;
	assignment=other.assignment;
	assProb = other.assProb;	
	return *this;
}

Assignment::Assignment(const Assignment& other):FM(other.FM),
	assignedVars(other.assignedVars),assignment(other.assignment),
	assProb(other.assProb),numOfVars(other.numOfVars){	
}

Assignment::Assignment():FM(*FormulaMgr::getInstance()){	
	numOfVars = FM.getNumVars();
	assignedVars.resize(numOfVars+1);
	assignment.resize(numOfVars+1);	
}

Assignment::Assignment(const DBS& relevantVars, const DBS& varsAssignment):FM(ContractedJunctionTree::getInstance()->getFormulaMgr()),
	assignedVars(relevantVars),	assignment(varsAssignment){
	numOfVars = ContractedJunctionTree::getInstance()->largestVar;	
}

//returns true if all assigned vars are set to 1
bool Assignment::allTrue() const{
	//means that every assigned var is assigned 1 in assignment.
	return assignedVars.is_subset_of(assignment);
}


string Assignment::print() const{
	std::stringstream ss;
	for(size_t i = assignedVars.find_first() ; i != boost::dynamic_bitset<>::npos ; i = assignedVars.find_next(i)){
		ss << i << ":" << assignment[i] << ", ";
	}
	ss << endl;
	return ss.str();	
}
