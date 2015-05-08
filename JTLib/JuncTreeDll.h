#ifndef JUNC_TREE_DLL_H
#define JUNC_TREE_DLL_H

#include <string>
#include "Params.h"
/*#ifdef JUNCTREEDLL"_EXPORTS*/
#ifdef __cplusplus
#define DLLEXPORT extern "C" __declspec(dllexport) 
#else 
#define DLLEXPORT __declspec(dllexport) 
#endif

#ifndef BUILDING_MODULE
	typedef signed int int32;
	typedef float float4;
	typedef int32 	varType;
	//typedef float4 	prob;
	typedef double 	prob;
	
	typedef struct varProbNode{
		varType var;
		float varProb;
		struct varProbNode *next;
	} varProbNode;

	typedef struct varNode{
		varType var;	
		struct varNode *next;
	} varNode;

	typedef struct DNFTermNode{
		varNode* termHead;
		struct DNFTermNode *next;
	} DNFTermNode;
	
	typedef struct junctionTreeState{
		varProbNode *varsAndProbs;
		DNFTermNode *DNFTerms;
	} junctionTreeState;

#endif
	
 long double performInference(junctionTreeState *jtState);
 long double performInferenceDebug(const char* strFormula, const char* varToProb);
 long double performInference_cnfFile(const char *cnfFilePath, unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF, const char* elimOrdrFile);
 long double performMPEInference_cnfFile(const char *cnfFilePath, unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF, const char* elimOrdrFile,int bound);
 void exportFormulaToHgrFile(const char * cnfFile, const char * outfile);

 void setParams(unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF, unsigned int seed, std::string dtreeFile, std::string weightsFile,
	std::string pmapFile, std::string csvOutFile, bool ADD_LIT_WEIGHTS);

 void readParams(int argc, char* argv[], Params** readParams);

#endif