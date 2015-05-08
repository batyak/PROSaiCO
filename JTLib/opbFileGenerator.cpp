#include "opbFileGenerator.h"
#include "Params.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
//#include <boost/filesystem.hpp>
#include "PBSAT_Interface.h"

bool OPBFileGenerator::getBoundByPBC(const Assignment& currAss, const DBS& relevantVars,
	const DBS& zeroedTerms, const DBS& relevantTerms, probType bound, probType& newBound){
		newBound = bound;
		if(!Params::instance().ADD_LIT_WEIGHTS){
			return true; //does nothing
		}
		#ifdef IS_WINDOWS //cannot run the library on windows
			return true;
		#endif

		stringstream opbFile;
		stringstream boundFile;
		try{
			OPBFileGenerator gen(currAss,relevantVars,zeroedTerms, relevantTerms,bound);			
			int randNum = rand();
			opbFile << "gen" << randNum << ".opb";
			gen.generateFile(opbFile.str());

			boundFile << "gen" << randNum << ".bound";
			
			#ifndef IS_WINDOWS
				PBSAT_IF PBSatExecution;
				PBSatExecution.PB_SAT(opbFile.str().c_str(),boundFile.str().c_str());			
			#endif
	
			//if for some reason, the bound file was not generated
		/*	if(!boost::filesystem::exists(boundFile.str())){
				if(boost::filesystem::exists(opbFile.str())){
					boost::filesystem::remove(opbFile.str());
				}
				return true;
			}*/

			//now read the bound
			ifstream boundStream(boundFile.str());
			string line;
			bool isSAT = true;
			while(getline(boundStream,line) != NULL){		
				line.shrink_to_fit();
				char firstLetter = line[0];
				switch(firstLetter){
					case 'c':{ //comment line					
						continue;}					
					case 's' :{
							string SATStr = line.substr(2);
							if(!boost::iequals(SATStr,"SATISFIABLE")){
								isSAT = false; //not SAT								
							}			
							break;
						}
					case 'v':{
						if(isSAT){
							string SATAssignmentStr = line.substr(2);
							newBound = getAssignmentWeightFromString(SATAssignmentStr);							
							}
						}
				}			
			}	
		/*	if(boost::filesystem::exists(opbFile.str())){
				boost::filesystem::remove(opbFile.str());
			}
			if(boost::filesystem::exists(boundFile.str())){
				boost::filesystem::remove(boundFile.str());
			} */
			return isSAT;

		}
		catch(...){
			/*if(boost::filesystem::exists(opbFile.str())){
				boost::filesystem::remove(opbFile.str());
			}
			if(boost::filesystem::exists(boundFile.str())){
				boost::filesystem::remove(boundFile.str());
			}*/
		}

}


/*
This method receives a string in the output format
-x1 x2 x3 -x4 -x8
and returns the probability of this assignment (x1=0,x2=x3=1, x4=x8=0)
*/
probType OPBFileGenerator::getAssignmentWeightFromString(string AssignmentString){
	Assignment returnedAss;
	trim(AssignmentString);
	vector<string> assignmentVector;
	//assumption: the delimiter is a single space
	//by stackoverflow: http://stackoverflow.com/questions/1301277/c-boost-whats-the-cause-of-this-warning
	//i can ignore the warning at this line.
	split(assignmentVector,AssignmentString,is_any_of(" "),token_compress_on);
	for(vector<string>::iterator it = assignmentVector.begin(); it != assignmentVector.end(); ++it) {
		string alllitStr = *it;
		varType lit;		
		bool neg = (alllitStr[0]=='-');
		string litStr = (neg ?  alllitStr.substr(2) :  alllitStr.substr(1)); //read after the "-x" in neg case or just after "x" in pos case		
		lit = lexical_cast<varType>(litStr); //convert
		lit = (neg ? -lit : lit); //make sure has correct sign
		returnedAss.setVar(lit,true);
	}
	return returnedAss.getAssignmentProb().getVal();
}