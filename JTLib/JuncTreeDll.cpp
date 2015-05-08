// JuncTreeDll.cpp : Defines the exported functions for the DLL application.
//
#include "JuncTreeDll.h"
#include "DirectedGraph.h"
#include "ConfigurationIterator.h"
#include "Utils.h"
#include "ContractedJunctionTree.h"
#include "CDCLFormula.h"
#include <stdexcept>
#include <string>
#include <time.h>
#include <math.h>
#include "Params.h"
#include "MsgGenerator.h"
#include "MsgGeneratorFactory.h"
#include <fstream>
#include <boost/thread.hpp>
#include "DTreeNode.h"
#include "argvparser.h"
#include "MPEMsgBuilder.h"

using namespace CommandLineProcessing;

#define TIME_LIMIT_SEC 10*60*60 //10 hours
#define OUT_CSV_FILE "results.csv"
//currRoot->buildMessage(GA,0,zeroedClauses,irrelevantVars);
class PRFunctor{
public:
	PRFunctor(TreeNode* currRoot, Assignment& GA, double lb, DBS& zeroedClauses, probType& retVal, Assignment* bestAssignment=NULL):
	currRoot(currRoot),GA(GA),lb(lb),zeroedClauses(zeroedClauses), retVal(retVal),bestAssignment(bestAssignment){
	}
	void operator()(){
		retVal = currRoot->buildMessage(GA,lb,zeroedClauses,bestAssignment);
	}
	
private:
	TreeNode* currRoot;
	Assignment& GA;
	double lb;
	DBS& zeroedClauses;	
	probType& retVal;
	Assignment* bestAssignment;
				
};
	static string getFilename(string filepath);
	void testFactors();
	void performUnitTests();
	 long double performInference(junctionTreeState *jtState){
		//clock_t t = clock();
		//ContractedJunctionTree* cjt = new ContractedJunctionTree(*jtState);
		//cjt->initRandomly();
		//t = clock() - t;	
		//double init_seconds = ((double)t - cjt->exportTimeMSec)/CLOCKS_PER_SEC;
		//cout << " Time to triangulate and initialize: " <<  init_seconds << " seconds"<< endl;	
	
		/*
		t = clock();
		prob prob = cjt->performInference();
		t = clock() - t;
		double inf_seconds =  ((double)t)/CLOCKS_PER_SEC;
		cout << " time to perform inference: " << inf_seconds << " seconds"<< endl;
		cout << " calculated prob: " <<  prob << endl;		
		cout << " Actual max number of entries: " << TreeNode::maxNumOfEntries << endl;

		return prob; */
		return 0.0;
	}

	bool isFileEmpty(string filePath){
		std::ifstream inFile(filePath,std::ios_base::in);
		if(!inFile){//non-existent - must be empty
			return true;
		}
		bool retVal= (inFile.peek()==std::ifstream::traits_type::eof());
		inFile.close();
		return retVal;

	}

	/*
	ContractedJunctionTree* createTree(FormulaMgr* FM){		
		clock_t t = clock();
		ContractedJunctionTree* cjt = new ContractedJunctionTree(FM);		
		t = clock() - t;	
		double treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to create tree: " <<  treeCreation << " seconds"<< endl;		
		t = clock();
		cjt->initRandomly();
		t = clock() - t;	
		treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to initialize: " <<  treeCreation << " seconds"<< endl;		
		return cjt;
	}
	*/
	
	ContractedJunctionTree* createTree(const char *cnfFilePath_cstr, const char* elimOrderFile){
		DTreeNode* dtreeRoot = NULL;
		string elimOrder = "";
		if(Params::instance().dtreeFile != ""){
			dtreeRoot = DTreeNode::generateFromFile(Params::instance().dtreeFile,cnfFilePath_cstr);
			elimOrder = dtreeRoot->elim_order;

		}
		
		std::string cnfFilePath(cnfFilePath_cstr, strlen(cnfFilePath_cstr));
		std::cout << "parsing cnf file " << cnfFilePath <<endl;
		string filePath(cnfFilePath_cstr);
		clock_t t = clock();
		ContractedJunctionTree* cjt = new ContractedJunctionTree(filePath,elimOrderFile,elimOrder);		
		t = clock() - t;	
		double treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to create tree: " <<  treeCreation << " seconds"<< endl;		
		if(cjt->getFormulaMgr().getTerms().size() == 0){
			return cjt;
		}
		t = clock();
		cjt->getFormulaMgr().generateLitToTermsMap();
		if(!elimOrder.empty()){
			cjt->initByDtreeRoot(dtreeRoot->cutset);
		}
		else{
			cjt->initRandomly();
		}
		t = clock() - t;	
		treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to initialize: " <<  treeCreation << " seconds"<< endl;		
		return cjt;
	}
	
	void printRuntimeStats_(ContractedJunctionTree* cjt){
		//DEBUG 
		//calculate the largest context
		double maxFactor=0;
		double factorSum =0;
		double maxFactorZeroedEntries=0;
		double zeroedEntriesSum =0;

		nodeIdSet cliqueNodeIds;
		nodeIdSet msgNodeIds;
		cjt->getCliquNodeIds(cliqueNodeIds,false);
		cjt->getMsgNodeIds(msgNodeIds);
		MSGGenerator* largestFactorMSGObj = NULL;
		for(nodeIdSet::const_iterator nit = cliqueNodeIds.cbegin() ; nit != cliqueNodeIds.cend() ; ++nit){
			CliqueNode* cn = (ContractedJunctionTree::getInstance())->getCliqueNode(*nit);
			MSGGenerator* msgGenObj = cn->getMSGGenerator();
			maxFactor =( msgGenObj->getFactorSize() > maxFactor) ? msgGenObj->getFactorSize() : maxFactor;
			factorSum+=msgGenObj->getFactorSize();
			zeroedEntriesSum+=msgGenObj->getZeroedEntries();
			largestFactorMSGObj = (largestFactorMSGObj == NULL || (msgGenObj->getFactorSize() > largestFactorMSGObj->getFactorSize())) ?
				msgGenObj : largestFactorMSGObj;
		}		
		for(nodeIdSet::const_iterator nit = msgNodeIds.cbegin() ; nit != msgNodeIds.cend() ; ++nit){
			TreeNode* tn = (ContractedJunctionTree::getInstance())->getMsgNode(*nit);
			MSGGenerator* msgGenObj = tn->getMSGGenerator();
			maxFactor =( msgGenObj->getFactorSize() > maxFactor) ? msgGenObj->getFactorSize() : maxFactor;
			factorSum+=msgGenObj->getFactorSize();
			zeroedEntriesSum+=msgGenObj->getZeroedEntries();
			largestFactorMSGObj = (largestFactorMSGObj == NULL || (msgGenObj->getFactorSize() > largestFactorMSGObj->getFactorSize())) ?
				msgGenObj : largestFactorMSGObj;
		}				
		cout << " Max Factor size is " << maxFactor <<  " in TW terms: " << "2^" << (log(maxFactor)/log(2.0))  << endl << 
			" out of which there are " << largestFactorMSGObj->getZeroedEntries() << " entries which were impiled 0 " << endl;
		cout << " Sum of all factors is " << factorSum <<  " out of which " << zeroedEntriesSum << " are implied zero" << endl;
		size_t numOfNodes = cliqueNodeIds.size()+msgNodeIds.size();
		cout << " AVG factor size is  " << (factorSum/numOfNodes) <<  endl;		
		cout << " Total number of clauses learned is: " << CDCLFormula::numOfLearnedClauses << endl;
		cout << " Total number backtracks: " << TreeNode::numOfBTs << endl;
		cout << " #tree-CPT prunes : " << Utils::treeCPTPrunes << endl;

		std::ofstream outfile;
		if(!Params::instance().csvOutFile.empty()){
			string outFileStr = Params::instance().csvOutFile;
			outfile.open(outFileStr, std::ios_base::app);
			double EFF_W = (log(maxFactor)/log(2.0));
			double AVG = (factorSum/numOfNodes) ;
			double AVG_LOG = (log((double)AVG)/log(2.0));
		
			outfile << EFF_W << "," << AVG << "," << AVG_LOG << "," << endl;
			outfile.close();
		}
	}

	void printRuntimeStats(ContractedJunctionTree* cjt){
		//DEBUG 
		//calculate the largest context
		double maxFactor=0;
		double factorSum =0;
		double maxFactorZeroedEntries=0;
		double zeroedEntriesSum =0;

		nodeIdSet cliqueNodeIds;
		nodeIdSet msgNodeIds;
		cjt->getCliquNodeIds(cliqueNodeIds,false);
		cjt->getMsgNodeIds(msgNodeIds);
		TreeNode* largestFactorNode = NULL;
		for(nodeIdSet::const_iterator nit = cliqueNodeIds.cbegin() ; nit != cliqueNodeIds.cend() ; ++nit){
			CliqueNode* cn = (ContractedJunctionTree::getInstance())->getCliqueNode(*nit);
			maxFactor =( cn->getFactorSize() > maxFactor) ? cn->getFactorSize() : maxFactor;
			factorSum+=cn->getFactorSize();
			zeroedEntriesSum+=cn->getZeroedEntries();
			largestFactorNode = (largestFactorNode == NULL || (cn->getFactorSize() > largestFactorNode->getFactorSize())) ?
				cn : largestFactorNode;
		}		
		for(nodeIdSet::const_iterator nit = msgNodeIds.cbegin() ; nit != msgNodeIds.cend() ; ++nit){
			TreeNode* tn = (ContractedJunctionTree::getInstance())->getMsgNode(*nit);
			maxFactor =( tn->getFactorSize() > maxFactor) ? tn->getFactorSize() : maxFactor;
			factorSum+=tn->getFactorSize();
			zeroedEntriesSum+=tn->getZeroedEntries();
			largestFactorNode = (largestFactorNode == NULL || (tn->getFactorSize() > largestFactorNode->getFactorSize())) ?
				tn : largestFactorNode;
		}				
		cout << " Max Factor size is " << maxFactor <<  " in TW terms: " << "2^" << (log(maxFactor)/log(2.0))  << endl << 
			" out of which there are " << largestFactorNode->getZeroedEntries() << " entries which were impiled 0 " << endl;
		cout << " Sum of all factors is " << factorSum <<  " out of which " << zeroedEntriesSum << " are implied zero" << endl;
		size_t numOfNodes = cliqueNodeIds.size()+msgNodeIds.size();
		cout << " AVG factor size is  " << (factorSum/numOfNodes) <<  endl;		
		cout << " Total number of clauses learned is: " << CDCLFormula::numOfLearnedClauses << endl;
		cout << " Total number backtracks: " << TreeNode::numOfBTs << endl;
		cout << " #tree-CPT prunes: " << Utils::treeCPTPrunes << endl;

		std::ofstream outfile;
		if(!Params::instance().csvOutFile.empty()){
			string outFileStr = Params::instance().csvOutFile;
			outfile.open(outFileStr, std::ios_base::app);
			double EFF_W = (log(maxFactor)/log(2.0));
			double AVG = (factorSum/numOfNodes) ;
			double AVG_LOG = (log((double)AVG)/log(2.0));
		
			outfile << EFF_W << "," << AVG << "," << AVG_LOG << ",";
			outfile.close();
		}
	}

	long double performPRInference_cnfFile(const char *cnfFilePath_cstr, unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF,const char* elimOrdrFile){
		Params::instance().CDCL = CDCL;
		Params::instance().timeoutSec = timeoutSec;
		Params::instance().DYNAMIC_SF = DYNAMIC_SF;		
		probType logPr = 0.0;
		probType evidenceProb = 1.0;
		probType retVal = 1.0;
		clock_t t = clock();		
		/*
		FormulaMgr* FM = new FormulaMgr(cnfFilePath_cstr);
		
		if(FM->getEvidenceProb() == 0.0){
			return 0.0;
		}*/
		ContractedJunctionTree* cjt = createTree(cnfFilePath_cstr,elimOrdrFile);
		//ContractedJunctionTree* cjt = createTree(FM);		
		//cjt->initRandomly();
		
		t = clock() - t;	
		double DS_creation =  ((double)t)/CLOCKS_PER_SEC;

		evidenceProb = cjt->getEvidenceProb();
				
		if(evidenceProb > 0.0 && !cjt->getFormulaMgr().getTerms().empty()){
			//cjt->getFormulaMgr().generateLitToTermsMap();
		
			t = clock();
			const std::list<TreeNode*> roots = cjt->getRoots();
			MSGGeneratorFactory factory;
			for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
				factory.initAlgForTree(*rootIt,MSGGeneratorFactory::PR);
			}
			t = clock() - t;	
			double PRInit = ((double)t)/CLOCKS_PER_SEC;
			cout << " Time to initialize PR algorithm : " <<  PRInit  << " seconds"<< endl;		

		
			Assignment GA;
			t = clock();		
			DBS zeroedClauses;
			zeroedClauses.resize(cjt->getFormulaMgr().getNumTerms());			
			Assignment MPEAssignment;
			for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
				zeroedClauses.reset();				
				BacktrackSM::reset();
				TreeNode* currRoot = *rootIt;		
				probType currRootProb;				
				PRFunctor w(currRoot,GA,0,zeroedClauses,currRootProb);
				boost::thread workerThread(w);
				if(workerThread.timed_join(boost::posix_time::seconds(Params::instance().timeoutSec))){				
					retVal *= currRootProb;	 //different trees are independent --> simply multiply their probabilities	
				}
				else{
					cout << " Method timed out after " << Params::instance().timeoutSec << " seconds" <<endl;
					return 0.0;
				}
			//	cout << "The processed tree: " << currRoot->printSubtree() << endl;		
				//probType currRootProb = currRoot->buildMessage(GA,0,zeroedClauses,irrelevantVars);
				//retVal *= currRootProb;	 //different trees are independent --> simply multiply their probabilities						
			}
			retVal*=evidenceProb;
		}else{
			cout << " evidence determined assignment to all other vars" << endl;
			retVal = evidenceProb;
		}

		logPr = log(retVal);

		
		t = clock() - t;

		cout << " evidence Prob " << cjt->getEvidenceProb() << endl;		
		cout << " calculated prob: " <<  retVal << " log_e(Prob) " << logPr << endl;		
		double PR_seconds =  ((double)t)/CLOCKS_PER_SEC;
		cout << " time to perform PR: " << PR_seconds << " seconds"<< endl;		
		cout << " Total Num Of Entries processed: " << MSGGenerator::totalNumOfEntries << endl;				
		cout << " Largest factor is : " << cjt->getNaiveLargestTreeFactor() << endl;
		cout << " Tree has " << cjt->getNumOfNodes() << " nodes" <<endl;
		cout << " total time: " << PR_seconds+DS_creation << " seconds" << endl;

	
		//print to out csv file
		std::ofstream outfile;
		if(!Params::instance().csvOutFile.empty()){
			string outFileStr = Params::instance().csvOutFile;
			outfile.open(outFileStr, std::ios_base::app);		
			string filepath(cnfFilePath_cstr);
			string filename = getFilename(filepath);
			size_t CNF_W = cjt->largestTreeNode;		
			if(isFileEmpty(outFileStr)){
				outfile << "File, seed, Prob, log_e(Prob), PR_seconds, PR_Init, PR_Total, CNF_W, EFF_W, AVG, log(AVG)" << endl;
			}
		
			outfile << filename << "," << Params::instance().seed << ","  << retVal <<","  << logPr << "," << PR_seconds << "," << DS_creation << "," << PR_seconds+DS_creation << "," << CNF_W << ",";		
			outfile.close();
		}

		if(retVal != evidenceProb){
			printRuntimeStats_(cjt);
		}
		return retVal;
	}


	bool validateMPESolution(const Assignment& MPEAss, FormulaMgr& FM, probType calcRes){
		const std::vector<CTerm>& terms = FM.getTerms();
		const DBS& assignedVars = MPEAss.getAssignedVars();
		const DBS& assignment = MPEAss.getAssignment();
		probType MPEProb = 1.0;		
		bool retVal = true;
		int numOfTermsSAT = 0;
		int numOfZeroedTerms =0;
		for(int i=0 ; i< terms.size() ; i++){
			const CTerm& term = terms[i];
			if(term.satisfiedByAssignment(MPEAss)){				
				numOfTermsSAT++;
			}
			if(term.zeroedByAssignment(MPEAss)){
				numOfZeroedTerms++;
			}
		}
		retVal= (numOfTermsSAT==0);
		if(!retVal)
			cout << " Assignment satisfies " << numOfTermsSAT << " terms " << endl;
		if(numOfZeroedTerms < terms.size()){
				retVal = false;
				cout << " Assignment does not zero " << terms.size()-numOfZeroedTerms << " terms " << endl;
		}

		cout << " Num Of vars: " << FM.getNumVars() << endl;
		cout << " Num Of assigned vars: " << assignedVars.count() << endl;

		CombinedWeight weight;
		for(size_t var = assignedVars.find_first() ; var != boost::dynamic_bitset<>::npos ; var = assignedVars.find_next(var)){	
			varType varT=(varType)var;
			weight*=(assignment[var] ? FM.getProb(varT) : FM.getProb(-varT));
		}
		if(!Utils::essentiallyEqual(weight.getVal(),calcRes)){
			cout << " weight.getVal() = " << weight.getVal() << " calcRes = " << calcRes << endl;
			retVal = false;
		}
		/*
		if(Params::instance().ADD_LIT_WEIGHTS){			
			probType posVarsTotalWeight = 0;
			probType negVarsTotalWeight = 0;
			probType actualWeight = 0;
			for(size_t var = assignedVars.find_first() ; var != boost::dynamic_bitset<>::npos ; var = assignedVars.find_next(var)){				
				if(assignment[var]){
					posVarsTotalWeight+=FM.getProb(var);
					actualWeight+= FM.getProb(var);
				}
				else{
					negVarsTotalWeight+=FM.getProb(var);
					actualWeight+= FM.getProb(-var);
				}
			}
			cout << " total pos weight: " << posVarsTotalWeight << endl;
			cout << " total neg weight: " << negVarsTotalWeight << endl;
			cout << " actual weight: " << actualWeight << endl;
		}*/
		return retVal;
	}

	static string getFilename(string filepath){
		// Remove directory if present.
		// Do this before extension removal incase directory has a period character.
		const size_t last_slash_idx = filepath.find_last_of("\\/");
		if (std::string::npos != last_slash_idx)
		{
			filepath.erase(0, last_slash_idx + 1);
		}

		// Remove extension if present.
		const size_t period_idx = filepath.rfind('.');
		if (std::string::npos != period_idx)
		{
			filepath.erase(period_idx);
		}
		return filepath;
	}
	 void readParams(int argc, char* argv[],Params** readParams){
		*readParams=NULL;
		ArgvParser cmd;
		cmd.setHelpOption();
		cmd.setIntroductoryDescription("WMC_JT version 1.0, June 2014, copyright 2014, Technion");
		cmd.defineOption("cnfFile","",ArgvParser::OptionRequired);
		cmd.defineOptionAlternative("cnfFile","f");

		cmd.defineOption("timeout"," timeout in seconds",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("timeout","t");

		cmd.defineOption("CDCL"," apply conflic directed clause learning",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("CDCL","l");

		cmd.defineOption("seed"," seed for rerunning",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("seed","s");

		cmd.defineOption("dtreeFile","file containing dtree",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("dtree","r");


		cmd.defineOption("pmapFile","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("pmapFile","p");

		cmd.defineOption("addMode","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("addMode","a");

		cmd.defineOption("bound","",ArgvParser::OptionRequiresValue);
		cmd.defineOptionAlternative("bound","b");

		cmd.defineOption("ibound","",ArgvParser::OptionRequiresValue);
	

		int errorCode = cmd.parse(argc,argv);
		if(errorCode != ArgvParser::NoParserError){
			cout << cmd.parseErrorDescription(errorCode) << endl;			
			return;
		}

		Params::instance().loadFromProps(cmd);
		*readParams=Params::x_instance();
	}


	 void setParams(unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF, unsigned int seed, string dtreeFile, 
		string weightsFile, string pmapFile, std::string csvOutFile, bool ADD_LIT_WEIGHTS){
		Params::instance().CDCL = CDCL;
		Params::instance().timeoutSec = timeoutSec;
		Params::instance().DYNAMIC_SF = DYNAMIC_SF;
		Params::instance().seed = seed;
		Params::instance().dtreeFile = dtreeFile;
		Params::instance().pmapFile = pmapFile;
		Params::instance().csvOutFile = csvOutFile;
		Params::instance().ADD_LIT_WEIGHTS = ADD_LIT_WEIGHTS;
	}
	/*
	Fills uper bound cache using the "mini-bucket" approach with the iBound
	*/
	void fillUpperBoundCache(const list<TreeNode*>& roots, size_t numOfTerms, double MPE_bound){
		if(Params::instance().ibound <= 0){
			return;
		}
		clock_t t = clock();
		Assignment GA;				
		DBS zeroedClauses;
		zeroedClauses.resize(numOfTerms);	
		for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
			zeroedClauses.reset();				
			BacktrackSM::reset();
			TreeNode* currRoot = *rootIt;						
			double lb = MPE_bound;
			probType res = currRoot->buildMessage(GA,lb,zeroedClauses);			
			zeroedClauses.reset();				
		}		

		for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
			TreeNode* root = *rootIt;
			MPEMsgBuilder* builder=	(MPEMsgBuilder*)root->getMSGGenerator();
			builder->clearCacheSaveUP();
		}

		t = clock()-t;
		double fillUPCache = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to fill upper bound cache with iBound=" <<  Params::instance().ibound << " is " <<  fillUPCache  << " seconds"<< endl;	
		Params::instance().ibound=0; //reset so that now ibound=inf will be executed
	}

	 long double performMPEInference_cnfFile(const char *cnfFilePath_cstr, unsigned int timeoutSec, bool CDCL,
		bool DYNAMIC_SF, const char* elimOrdrFile, int bound){			
		Utils::treeCPTPrunes =0;
//		Params::instance().CDCL = CDCL;
//		Params::instance().timeoutSec = timeoutSec;
//		Params::instance().DYNAMIC_SF = DYNAMIC_SF;		
		double fillUPCache=0;
		probType logPr = 0.0;
		probType evidenceProb = 1.0;
		CombinedWeight retVal;	
		clock_t t = clock();		

		ContractedJunctionTree* cjt = createTree(cnfFilePath_cstr,"");
		cjt->getFormulaMgr().setAllVarsMP();
		t = clock() - t;	
		double DS_creation =  ((double)t)/CLOCKS_PER_SEC;
		cout << " DS_creation : " <<  DS_creation  << " seconds"<< endl;
		evidenceProb = cjt->getEvidenceProb();
		Assignment MPEAssignment;
		bool ranAlg = true;
		double MPE_bound;
		if(Params::instance().ADD_LIT_WEIGHTS){
			MPE_bound = (bound > 0) ? bound : HUGE_VAL;
		}
		else{
			MPE_bound = bound;
		}
		bool evidenceWorsethanBound = (Params::instance().ADD_LIT_WEIGHTS ? evidenceProb >= MPE_bound : evidenceProb <= MPE_bound);
		if(!evidenceWorsethanBound && !cjt->getFormulaMgr().getTerms().empty()){			
			t = clock();
			const std::list<TreeNode*>& roots = cjt->getRoots();
			MSGGeneratorFactory factory;
			for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
				factory.initAlgForTree(*rootIt,MSGGeneratorFactory::MPE);
			}
			t = clock() - t;	
			double MPEInit = ((double)t)/CLOCKS_PER_SEC;
			cout << " Time to initialize MPE algorithm : " <<  MPEInit  << " seconds"<< endl;		
			t=clock();
			fillUpperBoundCache(cjt->getRoots(),cjt->getFormulaMgr().getNumTerms(),MPE_bound);
			t=clock()-t;
			fillUPCache=((double)t)/CLOCKS_PER_SEC;
			cout << " fillUPCache : " <<  fillUPCache  << " seconds"<< endl;		
			Assignment GA;
			t = clock();		
			DBS zeroedClauses;
			zeroedClauses.resize(cjt->getFormulaMgr().getNumTerms());					
			for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
				zeroedClauses.reset();				
				BacktrackSM::reset();
				TreeNode* currRoot = *rootIt;	
				#ifdef _DEBUG
					//	cout << "The processed tree: " << currRoot->printSubtree() << endl;
				#endif
				probType currRootProb;
				double lb = MPE_bound;
				PRFunctor w(currRoot,GA,lb,zeroedClauses,currRootProb,&MPEAssignment);
				boost::thread workerThread(w);
				if(workerThread.timed_join(boost::posix_time::seconds(Params::instance().timeoutSec))){				
					retVal *= currRootProb;	 //different trees are independent --> simply multiply their probabilities	
				}
				else{
					cout << " Method timed out after " << Params::instance().timeoutSec << " seconds" <<endl;
					return 0.0;
				}								
				zeroedClauses.reset();				

			}
			retVal*=evidenceProb;
		}else{
			cout << " evidence determined assignment to all other vars" << endl;
			retVal = evidenceProb;
			ranAlg = false;
		}

		bool gotSol=(Params::instance().ADD_LIT_WEIGHTS ?  retVal.getVal() < MPE_bound : retVal.getVal() > MPE_bound);
		if(!gotSol) retVal.setVal(MPE_bound);

		logPr = retVal.getVal() > 0 ? log(retVal.getVal()) : 0.0;

		t = clock() - t;
		cout << " evidence Prob " << cjt->getEvidenceProb() << endl;	
		cout << " MPE prob: " <<  retVal.getVal() << 
			" ( MPEAssignment.getAssignmentProb()=" << MPEAssignment.getAssignmentProb().getVal() << endl;
		cout << " MPE log(prob): " <<  logPr << endl;

		//cout << " MPE assignment: " << MPEAssignment.print() << endl;
		double MPE_seconds =  ((double)t)/CLOCKS_PER_SEC;
		cout << " time to perform MPE: " << MPE_seconds << " seconds"<< endl;		
		cout << " Total Num Of Entries processed: " << MSGGenerator::totalNumOfEntries << endl;				
		cout << " Largest factor is : " << cjt->getNaiveLargestTreeFactor() << endl;
		cout << " Tree has " << cjt->getNumOfNodes() << " nodes" <<endl;
		cout << " Total number of prunes: " << MSGGenerator::numOfPrunes << endl;
		cout << " total time: " << MPE_seconds+DS_creation+fillUPCache << " seconds" << endl;
		probType expectedAssProb = (retVal/evidenceProb).getVal();
		bool valid = (!gotSol) || validateMPESolution(MPEAssignment,cjt->getFormulaMgr(),expectedAssProb);
		cout << " The MPE assignment is" << (valid ? "" : " not" ) << " valid" << endl;
		//print to out csv file
		std::ofstream outfile;
		if(!Params::instance().csvOutFile.empty()){
			string outFileStr = Params::instance().csvOutFile;
			outfile.open(outFileStr, std::ios_base::app);
			string filepath(cnfFilePath_cstr);
			string filename = getFilename(filepath);
			size_t CNF_W = cjt->largestTreeNode;		
			if(isFileEmpty(outFileStr)){
				outfile  << "File, seed, Prob, log_e(Prob), valid, MPE_seconds, MPEInit, MPETotal, CNF_W, EFF_W, AVG, log(AVG)" << endl;
			}		
			outfile << filename << ","  << Params::instance().seed << "," <<  retVal.getVal() <<"," << logPr << "," << valid << "," << MPE_seconds << "," << DS_creation << "," << MPE_seconds+DS_creation << "," << CNF_W << ",";		
			outfile.close();
		}
		if(ranAlg){
			printRuntimeStats_(cjt);
		}

		
		return retVal.getVal();
	}

	

	 long double performInference_cnfFile(const char *cnfFilePath_cstr,unsigned int timeoutSec, bool CDCL,bool DYNAMIC_SF,const char* elimOrdrFile){
		//set parameters
		return performPRInference_cnfFile(cnfFilePath_cstr, timeoutSec, CDCL, DYNAMIC_SF,elimOrdrFile);
		/*

		Params::CDCL = CDCL;
		Params::timeoutSec = timeoutSec;
		Params::DYNAMIC_SF = DYNAMIC_SF;
		std::string cnfFilePath(cnfFilePath_cstr, strlen(cnfFilePath_cstr));
		std::cout << "parsing cnf file " << cnfFilePath <<endl;
		string filePath(cnfFilePath_cstr);
		clock_t t = clock();
		ContractedJunctionTree* cjt = new ContractedJunctionTree(filePath);		
		t = clock() - t;	
		double treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to create tree: " <<  treeCreation << " seconds"<< endl;		
		t = clock();
		cjt->initRandomly();
		t = clock() - t;	
		treeCreation = ((double)t)/CLOCKS_PER_SEC;
		cout << " Time to initialize: " <<  treeCreation << " seconds"<< endl;		
		//DEBUG
		//cjt->validateTermDistInTree();
		
		Utils::setTimeLimit(TIME_LIMIT_SEC);
		Utils::setStartTime();
		t = clock();
		prob prob;		
		try{
			prob = cjt->performInference();
		}
		catch(Utils::TimeOutException&){			
			cout << " Application timed out after " << TIME_LIMIT_SEC << endl;
		}
		t = clock() - t;
		cout << " calculated prob: " <<  prob << endl;
		double inf_seconds =  ((double)t)/CLOCKS_PER_SEC;
		cout << " time to perform inference: " << inf_seconds << " seconds"<< endl;		
		cout << " Total Num Of Entries processed: " << TreeNode::totalNumOfEntries << endl;		
		cout << " Total number of calls " << TreeNode::totalCalls << " vs. number of cache hits: "<<  TreeNode::cacheHits;		
		cout << " Largest factor is : " << cjt->getNaiveLargestTreeFactor() << endl;
		cout << " Tree has " << cjt->getNumOfNodes() << " nodes";
		cout << " total time: " << inf_seconds+treeCreation << " seconds" << endl;
		
		//DEBUG 
		//calculate the largest context
		double maxFactor=0;
		double factorSum =0;
		double maxFactorZeroedEntries=0;
		double zeroedEntriesSum =0;

		nodeIdSet cliqueNodeIds;
		nodeIdSet msgNodeIds;
		ContractedJunctionTree::getInstance()->getCliquNodeIds(cliqueNodeIds,false);
		ContractedJunctionTree::getInstance()->getMsgNodeIds(msgNodeIds);
		TreeNode* largestFactorNode = NULL;
		for(nodeIdSet::const_iterator nit = cliqueNodeIds.cbegin() ; nit != cliqueNodeIds.cend() ; ++nit){
			CliqueNode* cn = (ContractedJunctionTree::getInstance())->getCliqueNode(*nit);
			maxFactor =( cn->getFactorSize() > maxFactor) ? cn->getFactorSize() : maxFactor;
			factorSum+=cn->getFactorSize();
			zeroedEntriesSum+=cn->getZeroedEntries();
			largestFactorNode = (largestFactorNode == NULL || (cn->getFactorSize() > largestFactorNode->getFactorSize())) ?
				cn : largestFactorNode;
		}		
		for(nodeIdSet::const_iterator nit = msgNodeIds.cbegin() ; nit != msgNodeIds.cend() ; ++nit){
			TreeNode* tn = (ContractedJunctionTree::getInstance())->getMsgNode(*nit);
			maxFactor =( tn->getFactorSize() > maxFactor) ? tn->getFactorSize() : maxFactor;
			factorSum+=tn->getFactorSize();
			zeroedEntriesSum+=tn->getZeroedEntries();
			largestFactorNode = (largestFactorNode == NULL || (tn->getFactorSize() > largestFactorNode->getFactorSize())) ?
				tn : largestFactorNode;
		}				
		cout << " Max Factor size is " << maxFactor <<  " in TW terms: " << "2^" << (log(maxFactor)/log(2.0))  << endl << 
			" out of which there are " << largestFactorNode->getZeroedEntries() << " entries which were impiled 0 " << endl;
		cout << " Sum of all factors is " << factorSum <<  " out of which " << zeroedEntriesSum << " are implied zero" << endl;
		size_t numOfNodes = cliqueNodeIds.size()+msgNodeIds.size();
		cout << " AVG factor size is  " << (factorSum/numOfNodes) <<  endl;		
		cout << " Total number of clauses learned is: " << CDCLFormula::numOfLearnedClauses << endl;
		cout << " Total number backtracks: " << TreeNode::numOfBTs << endl;
		return prob;
		/*
		cout << "The node with the largest factor is: " << largestFactorNode->shortPrint() << endl;;			
		cout << " Its graph " << largestFactorNode->getOrderConstraintGraph().printGraph();
		cout << " Its parent is: " << largestFactorNode->getParent()->shortPrint() << endl;
		cout << " With graph: " << largestFactorNode->getParent()->getOrderConstraintGraph().printGraph();
		cout << " parent->getFactorSize " << largestFactorNode->getParent()->getFactorSize() << endl;
		TreeNode* grandParent = largestFactorNode->getParent()->getParent();
		if(grandParent == NULL){
			cout << " grandparent is NULL" << endl;
		}
		else{
			cout << " GrandParent is: " << grandParent->shortPrint() << endl;
			cout << " grandparent graph: " << grandParent->getOrderConstraintGraph().printGraph();
		}
		cout << " grandparent->getFactorSize " << grandParent->getFactorSize() << endl;*/
		
	}
	static string printTermsDBS(const DBS& BS, string delim=", ", string end="", bool newline = true){
		std::stringstream ss;
		bool first = true;
		for(size_t i = BS.find_first() ; i != boost::dynamic_bitset<>::npos ; i = BS.find_next(i)){
			if(!first){
				ss << delim;
			}
			else{
				first = false;
			}
			ss << (i+1);
		}
		ss << " " << end;
		if(newline)
			ss << endl;
		return ss.str();	
	}

	size_t getNumOfEffectiveVars(const varToBS& litToTerms, int numOfVars){
		size_t retVal = 0;
		for(varType v = 1 ; v <= numOfVars ; v++){
			varToBS::const_iterator vit =  litToTerms.find(v);
			varToBS::const_iterator _vit =  litToTerms.find(-v);
			if(vit != litToTerms.end() || _vit != litToTerms.end())
				retVal++;
		}
		return retVal;
	}

	 void exportFormulaToHgrFile(const char * cnfFile, const char * outfile){
		string strCnfile(cnfFile);
		FormulaMgr FM(strCnfile,false); //reads file and updates FM data structures
		FM.generateLitToTermsMap();
		const varToBS& litToTerms = FM.getLitToTerms();
		ofstream out;
		out.open(outfile);
		//variables represent hyperedges and the vertices represent terms
		//#hyperedges #vertices #graph-type (weighted/unweighted)
		size_t numOfEffectiveVars = getNumOfEffectiveVars(litToTerms,FM.getNumVars());
		out << numOfEffectiveVars << " " << FM.getNumTerms() << " 0" << endl;
		stringstream hpred;
		for(varType v = 1 ; v <= FM.getNumVars() ; v++){			
			hpred.str(std::string());
			bool written = false;
			varToBS::const_iterator vit =  litToTerms.find(v);
			if(vit != litToTerms.end()){
				const DBS& vTerms = vit->second;
				string vTermsStr = printTermsDBS(vTerms," ","",false);
				hpred << vTermsStr;
				written = true;
			}
			varToBS::const_iterator _vit =  litToTerms.find(-v);
			if(_vit != litToTerms.end()){
				const DBS& vTerms = _vit->second;
				string _vTermsStr = printTermsDBS(vTerms," ","",false);
				hpred <<  _vTermsStr;
				written = true;
			}
			if(written){
				out << hpred.str() << " 0" << endl;
			}
		}
		out.close();
	}
	
	
