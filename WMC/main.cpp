// testApp.cpp : Defines the entry point for the console application.
//
#include "JuncTreeDll.h"
#include <string>
#include <iostream>
#include <sstream>
using namespace std;


int main(int argc, char* argv[])
{
//	cout << "WMC_JT version 1.0, June 2014" << endl
//		 << "copyright 2014, Technion" << endl;		
	

	Params* parameters;
	readParams(argc, argv, &parameters);
	if(parameters == NULL)
		return 0;
	if(parameters->ishelp){
		return 0;
	}
	char* filename = argv[1];
    cout <<"Solving " << parameters->cnfFile << " ......" << endl;
	cout << parameters->toString() << endl;

	//performMPEInference_cnfFile(filename,timeoutSec,CDCL,DYNAMIC_SF,orderFile,bound);	
	performInference_cnfFile(parameters->cnfFile.c_str(),
							 parameters->timeoutSec,
							 parameters->CDCL,
							 true,
							 "");	
	return 0;
}
/*
int main(int argc, char* argv[])
{
	char* filename = NULL;
	char* orderFile = "";    
	char* dtreeFile = "";    
	char* weightsFile = ""; 	
	char* determinedVarsFile = ""; //the pmap file containing the variables that are determined by indicator vars
	char* csvOutFile = ""; //the pmap file containing the variables that are determined by indicator vars
	bool clauseLearning = true;
	if(argc < 2){
		cout << "WMC_JT version 1.0, June 2014" << endl
			 << "copyright 2014, Technion" << endl
			 << "Usage: "<< argv[0] << " cnf_file [-t time_limit_seconds]" 
			 << "[-l clause_learning_on] [-d dynamic_formula_handling]" << endl;
		return 2;
	}
   
	cout << "WMC_JT version 1.0, June 2014" << endl
		 << "copyright 2014, Technion" << endl;		
	filename = argv[1];
    cout <<"Solving " << filename << " ......" << endl;
	int current = 1;
	bool CDCL = false;
	bool DYNAMIC_SF = true;
	unsigned int timeoutSec = 10000; 
	unsigned int seed = 0; 
	while (++current < argc)
	{
		switch (argv[current][1]){
		case 't': timeoutSec= atoi(argv[++current]);				
				  break; 
		case 'l':	CDCL = ((atoi(argv[++current])) > 0);
					break;
		case 'd': DYNAMIC_SF = ((atoi(argv[++current])) > 0);
					break;
		case 'o': orderFile = argv[++current];
					break;
		case 's': seed = atoi(argv[++current]);
					break;
		case 'r': dtreeFile = argv[++current];
					break;
		case 'w': weightsFile = argv[++current];
					break;
		case 'p': determinedVarsFile = argv[++current];
					break;
		case 'x': csvOutFile = argv[++current];
					break;
				
		}

	}
	std::stringstream ss;
	ss << " parameters: " << "timeoutSec=" << timeoutSec << " CDCL= " << CDCL << " DYNAMIC_SF= " << DYNAMIC_SF << " seed: " 
		<< seed;
	string dtreeFileStr(dtreeFile);
	if(!dtreeFileStr.empty())
		ss << "dtreeFile=" << dtreeFileStr;
	string pmapFile(determinedVarsFile);
	if(!pmapFile.empty())
		ss << "determinedVarsFile=" << pmapFile;
	string csvOut(csvOutFile);
	if(!csvOut.empty())
		ss << "csvOutFile=" << pmapFile;
	ss << endl;
	
	

	cout << " sizeof(double) " << sizeof(double) << "  sizeof(long double) " <<  sizeof(long double) << endl; 
	cout << ss.str();
	setParams(timeoutSec, CDCL,DYNAMIC_SF, seed,dtreeFile,weightsFile,determinedVarsFile,csvOut,false);
	performInference_cnfFile(filename,timeoutSec,CDCL,DYNAMIC_SF,orderFile);	
	return 0;
}

*/