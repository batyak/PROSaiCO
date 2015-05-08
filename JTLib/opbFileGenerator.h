
#ifndef OPB_GEN_H
#define OPB_GEN_H
#include "Definitions.h"
#include "Assignment.h"
#include "FormulaMgr.h"
#include <iostream>
#include <fstream>

// Check windows
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
	#define IS_WINDOWS
#endif



class OPBFileGenerator{	
public:
	
	OPBFileGenerator(const Assignment& currAss, const DBS& relevantVars,
			const DBS& zeroedTerms, const DBS& relevantTerms, probType bound): bound(bound){
		nonZeroedTerms = NULL;
		formulaVars = NULL;
		TOPWeight = 0;
		nonZeroedTerms = new DBS(relevantTerms);
		nonZeroedTerms->operator-=(zeroedTerms);
		formulaVars = new DBS(relevantVars);
		formulaVars->operator-=(currAss.getAssignedVars());
		largestVaridx = 0;
		FM = FormulaMgr::getInstance();
	}

	string getFormulaAsConstraints(){
		stringstream ss;
		for(size_t termIdx = nonZeroedTerms->find_first() ; termIdx != DBS::npos; termIdx =nonZeroedTerms->find_next(termIdx) ){
			getTermAsConstraint(termIdx,ss);
		}
		return ss.str();
	}
	
	void generateFile(string outfilePath){
		ofstream opbFile;		
		string formulaAsStr = getFormulaAsConstraints();
		string boundConstraint = getBoundConstraint(bound);
		string topCommentStr = topCommentLine(largestVaridx,nonZeroedTerms->count()+1);
		opbFile.open(outfilePath);
		opbFile << topCommentStr <<  formulaAsStr << boundConstraint;		
		opbFile.close();
	}
	virtual ~OPBFileGenerator(){
		if(nonZeroedTerms != NULL){
			delete nonZeroedTerms;
		}
		if(formulaVars != NULL){
			delete formulaVars;
		}
	}
	const std::string& GetPBCAlgPath(){
        // Initialize the static variable
     //   static std::string pathToPBCAlg("/lv_local/home/batyak/pb2sat+zchaff");
		#ifdef IS_WINDOWS	
			static std::string pathToPBCAlg("java -jar C:\\MyTools\\sat4j-pb\\sat4j-pb.jar ");
		#else
			static std::string pathToPBCAlg("java -jar /lv_local/home/batyak/sat4j-pb.jar ");
		#endif
        return pathToPBCAlg;
    }

	//get bound by Pseudo Boolean Constraints
	static bool getBoundByPBC(const Assignment& currAss, const DBS& relevantVars,
			const DBS& zeroedTerms, const DBS& relevantTerms, probType bound, probType& newBound);
private:
	static probType getAssignmentWeightFromString(string AssignmentString);
	/*ASSUMPTIONS: 
		1. termId represents an UNZEROED term
		Translate a term x1*(-x2)*(-x4)*x5 into constraint -1 x1 +1 x2 +1 x4 -1 x5 >= 1;		
	*/

	string topCommentLine(size_t largestVaridx, size_t numOfConstraints){
		stringstream topCommentLiness;
		topCommentLiness << "* #variable= " << largestVaridx << " #constraint= " << numOfConstraints << endl;
		return topCommentLiness.str();
	}

	void getTermAsConstraint(size_t termId, stringstream& ss){
		CTerm& term = FM->getTerm(termId);
		const vector<varType>& lits = term.getLits();		
		for(std::vector<varType>::size_type i = 0; i != lits.size(); i++) {
			 varType v = (lits[i] > 0 ? lits[i]: -lits[i]) ;
			 largestVaridx = (v > largestVaridx) ? v : largestVaridx;
			 if((*formulaVars)[v]){ //an unassigned var						 
				 ss << (lits[i] > 0 ? "-1" : "+1") << " x" << v << " ";				 
			 }
		}
		ss << ">= 1;" << endl;
		
	}

	/*
	The bound should be in the form +w1 y1 +w2 y2 ...+wm ym <= bound
	which is equivalent to
	-w1 y1 -w2 y2 ...-wm ym >= -bound

	this is done by iterating pover all the unassigned subtree vars 
	and adding them to the constraint if their weight is different than 0
	*/
	string getBoundConstraint(probType bound){
		stringstream ss;
		DBS non1Vars(*formulaVars);
		non1Vars.operator-=(FM->getWeight1Vars()); //all unassigned vars with weight  != 0
		for(size_t varidx = non1Vars.find_first() ; varidx != DBS::npos ; varidx = non1Vars.find_next(varidx)){
			//in the ADD case, the weighted vars weight is != on the positive lit
			probType varWeight = FM->getLitProb((varType)varidx);
			ss << "-" << varWeight << " x" << varidx << " ";
		}
		ss << ">= -" << bound << ";" << endl;
		return ss.str();
	}

	probType getTermWeight(const vector<varType>& lits){
		probType clauseWeight = 0;
		for(std::vector<varType>::size_type i = 0; i != lits.size(); i++) {
			 varType v = (lits[i] > 0 ? lits[i]: -lits[i]) ;
			 if((*formulaVars)[v]){ //an unassigned var		
				 //in the encoding to MPE, we added one negative lit to every clause, we want the probability that this lit is zeroed.
				 clauseWeight = FM->getLitProb(-lits[i]); //should be 0 in all but one literal		
				 if(clauseWeight > 0){
					 break;
				 }
			 }
		}
		return clauseWeight;
	}

	DBS* nonZeroedTerms;
	DBS* formulaVars;	
	FormulaMgr* FM;
	probType TOPWeight;
	probType bound;
	varType largestVaridx;
};













#endif
