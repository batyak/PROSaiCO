#include "PRMessageBuilder.h"
#include "ContractedJunctionTree.h"
#include "ConfigurationIterator.h"

PRMessageBuilder::PRMessageBuilder(TreeNode* TN): MSGGenerator(TN){
	cache = NULL;	
}

void PRMessageBuilder::initByTN(){	
	if(Params::instance().DYNAMIC_SF){		
		cache = new subFormulaCache(TN->getDNFIds(),TN->getSubtreeVars());				
	}
	else{
		cache = new assignmentCache(TN->getSubtreeVars());		
	}
			
}

probType PRMessageBuilder::getDNFTermProb(const Assignment& context) const{
	if(!TN->isDNFTerm()){ //not  DNF term,cannot be satisfied
		return 1.0;
	}	
	const varSet& vars = TN->getVars();
	varSet::const_iterator end = vars.end();
	probType satProb = 1.0;
	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		varType v = *it;
		Status vStat = context.getStatus(v);
		if(vStat == SET_F){ //DNF term must be falsified
			return 1.0;
		}
		if(vStat == UNSET){
			satProb*=FM.getLitProb(v);
		}
	}
	return (1.0 - satProb);	

}


probType PRMessageBuilder::buildMsg(Assignment& context,probType lb, DBS& zeroedClauses,Assignment* bestAssignment){
	//DEBUG
	//string contextStr = context.print();

	if(TN->isDNFTerm()){
		return getDNFTermProb(context);
	}
	probType cp=0;	
	bool inCache =false;		
	size_t cacheKey = 0;
	cacheKey = cache->getKey(context,zeroedClauses);	
	inCache = cache->get(cp,context,cacheKey);
	if(inCache){
		cacheHits++;
		return cp;
	}

	//if the subformula is SAT (UNSAT) then simply return 1.0
	const DBS& subtreeTerms = TN->getDNFIds();
	if(subtreeTerms.is_subset_of(zeroedClauses)){
		//in this case calculate the MPE assignment for the complete subtree
		//and return
		cache->set(1.0,context,cacheKey);
		return 1.0;
	}

	//set the node level for backtracking	
	if(Params::instance().CDCL){ 
		updateNodeLevel();
	}	
	//build formula only based on relevant DNF nodes (those that were not zeroed out by the parent assignment)	
	updateGlobalAssignment(context);
	varSet relevantVars;	

	getRelevantMembers(context,relevantVars,zeroedClauses);
	DirectedGraph* relevantOrderGraph = NULL;
	if(!relevantVars.empty()){
		relevantOrderGraph = getRelevantGraphNew(relevantVars,zeroedClauses);
	}
	probType p = 1.0;
	probType retVal = 0.0;	

	configuration_iterator endIt = configuration_iterator::end(*TN);	
	const list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();
	int numOfConfigs = 0;
	configuration_iterator configAt = configuration_iterator::begin(relevantOrderGraph,p,*TN);	

	DBS newZeroedClauses;
	if(Params::instance().DYNAMIC_SF){
		newZeroedClauses.resize(FM.getNumTerms(),false);		
	}
	
	for(; configAt != endIt ; ++configAt){		
		numOfConfigs++;		
		probType& tcfAssProb =configAt.assProb(); 
		if(tcfAssProb == 0.0){ //should NOT happen
			continue;//no point in processing an assignment whose probability is 0	
		}
		Assignment currAss((*configAt).A);	
		//string configStr = currAss.print();	

		if(Params::instance().DYNAMIC_SF){			
			//update set of zereod clauses
			newZeroedClauses.reset();
			calculateZeroedClauses(currAss,newZeroedClauses);						
			newZeroedClauses.operator-=(zeroedClauses);
			zeroedClauses.operator|=(newZeroedClauses);			
		}

		context.setOtherAssignment(currAss);	
		factorSize++;
		totalNumOfEntries++;				
		for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){
			TreeNode* currChild = *child;
			probType childProb = currChild->buildMessage(context,1.0,zeroedClauses);
			if(childProb == 0.0){ //the prob value of this entry will remain 0 regardless of the prob. values received by the other children							
				zeroedEntries++;															
			}
			tcfAssProb *= childProb;
			if(tcfAssProb == 0.0){
				break;
			}
		}

		context.clearOtherAssignment(currAss);		
		if(Params::instance().DYNAMIC_SF){
			zeroedClauses.operator-=(newZeroedClauses);//ok , the two sets are disjoint
		}

		//the parent assignment requires BT
		if(tcfAssProb == 0.0 && BacktrackSM::getInstance()->BacktrackRequired(context)){ //current assignment falsifies the learned clause						
			numOfBTs++;
			break;			
		}	

		retVal += tcfAssProb;	
	}
//	irrelevantVars.operator-=(local_context_irrelevant_vars);	
	cache->set(retVal,context,cacheKey);
	delete relevantOrderGraph;
	AssProb AP = configAt.getAggregateAP();

	return retVal;
}