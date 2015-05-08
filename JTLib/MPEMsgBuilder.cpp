#include "MPEMsgBuilder.h"
#include "ContractedJunctionTree.h"
#include "ConfigurationIterator.h"


MPEMsgBuilder::MPEMsgBuilder(TreeNode* TN): MSGGenerator(TN){
	Cache = NULL;	
	upperBoundCache = NULL;
	ADD_VARS = Params::instance().ADD_LIT_WEIGHTS;	
	assignedChildVars.resize((FM.getNumVars()+1),false);
	unassignedChildVars.resize((FM.getNumVars()+1),false);
}

void MPEMsgBuilder::updateBestAssignment(Assignment& context, DBS& zeroedClauses, Assignment& MPEAss){		
	if(TN->isDNFTerm()){
		//getDNFTermMPProb(context, &MPEAss);
		return;
	}
	Assignment CachedMPEAss;
	MPECacheStorage* cacheStore = NULL;
	size_t cacheKey = Cache->getKey(context,zeroedClauses);	
	cacheStore = Cache->get(context,zeroedClauses, cacheKey);
	DBS newZeroedClauses;
	newZeroedClauses.resize(FM.getNumTerms(),false);
	newZeroedClauses.reset();	
	
	if(cacheStore != NULL){				
		if(Cache->numEntries() < 1)
			cout << " somethinf wrong with Cache->numEntries()" <<endl;
#ifdef _DEBUG
		bool subset = (TN->getDNFIds().is_subset_of(zeroedClauses));		
		if(MPEAss.getAssignedVars().intersects(cacheStore->MPEAss->getAssignedVars())){
			DBS wronglyAssigned = cacheStore->MPEAss->getAssignedVars();
			wronglyAssigned.operator&=(MPEAss.getAssignedVars());
			for(size_t v = wronglyAssigned.find_first() ; v != DBS::npos ; v = wronglyAssigned.find_next(v)){
				bool allVarClausesZerob = allVarClausesZero((varType)v,zeroedClauses);
				const DBS& parentVars = (TN->getParent() != NULL) ? TN->getParent()->getVarsBS() : TreeNode::emptyDBS;
				bool belongsToParent = (!parentVars.empty() && parentVars[v]);
				if(!allVarClausesZerob || belongsToParent){
					if(belongsToParent){
						MSGGenerator* pGen = TN->getParent()->getMSGGenerator();
						DBS vTerms = pGen->getTermsWithLit((varType)v);
						vTerms.operator|=(pGen->getTermsWithLit(-(varType)v));
						bool isolated = pGen->isTermsSetIsolated(vTerms,v);
					}
					cout << "should have been assigned" << endl;
				}
			}
		}
		if(context.getAssignedVars().intersects(cacheStore->MPEAss->getAssignedVars())){
			DBS wronglyAssigned = cacheStore->MPEAss->getAssignedVars();
			wronglyAssigned.operator&=(context.getAssignedVars());
			for(size_t v = wronglyAssigned.find_first() ; v != DBS::npos ; v = wronglyAssigned.find_next(v)){
				bool allVarClausesZerob = allVarClausesZero((varType)v,zeroedClauses);
				const DBS& parentVars = (TN->getParent() != NULL) ? TN->getParent()->getVarsBS() : TreeNode::emptyDBS;
				bool belongsToParent = (!parentVars.empty() && parentVars[v]);
				if(!allVarClausesZerob || belongsToParent){
					if(belongsToParent){
						MSGGenerator* pGen = TN->getParent()->getMSGGenerator();
						DBS vTerms = pGen->getTermsWithLit((varType)v);
						vTerms.operator|=(pGen->getTermsWithLit(-(varType)v));
						bool isolated = pGen->isTermsSetIsolated(vTerms,v);
					}
					cout << "should have been assigned" << endl;
				}
			}
		}
#endif		
		
		calculateZeroedClauses(*cacheStore->MPEAss,newZeroedClauses);
		newZeroedClauses.operator-=(zeroedClauses); //now contains only the zereod clauses following the currAss	
		zeroedClauses.operator|=(newZeroedClauses);			
		MPEAss.setOtherAssignment(*cacheStore->MPEAss);
		CachedMPEAss=*cacheStore->MPEAss;
		const DBS& subtreeTerms = TN->getDNFIds();
		if(subtreeTerms.is_subset_of(zeroedClauses)){	
			//make sure that all subtree vars are UNSAT
			zeroedClauses.operator-=(newZeroedClauses);
			return;
		}	
	}
	else{
		//validate that tyhe subformula is UNSAT
		const DBS& subtreeTerms = TN->getDNFIds();
		if(subtreeTerms.is_subset_of(zeroedClauses)){
			return;
		}	
		else{
			size_t numCacheEntries = Cache->numEntries();		
			size_t numUPCacheEntries=upperBoundCache->numEntries();
			if(numCacheEntries==0){			
				cout << " cache contains 0  entries and " << numUPCacheEntries << " UP entries " << endl;
			}
			for(size_t termIdx = subtreeTerms.find_first() ; termIdx != DBS::npos ; termIdx = subtreeTerms.find_next(termIdx)){
				CTerm& t = FM.getTerm(termIdx);
				if(!t.zeroedByAssignment(context) && !t.zeroedByAssignment(MPEAss)){
					//cout << " cache store == NULL and the subformula is not SAT" << endl;			
				}
			}			
		}
		//return;
	}
	
		
	const list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();
	//Assignment localCpy(context); //so children do not interfere
	assert(context.getAssignedVars().intersects(CachedMPEAss.getAssignedVars())==false);
	//localCpy.setOtherAssignment(*(cacheStore->MPEAss));
	context.setOtherAssignment(CachedMPEAss);
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){
		TreeNode* childNode = *child;		
		childNode->getMSGGenerator()->updateBestAssignment(context, zeroedClauses, MPEAss);
	}
	//validate that all current node vars have been assigned
	DBS unassignedCurrVars = TN->getVarsBS();
	unassignedCurrVars.operator-=(MPEAss.getAssignedVars());
	setUnaffectingVarsMPE(zeroedClauses,unassignedCurrVars,MPEAss);
	context.clearOtherAssignment(CachedMPEAss);
	zeroedClauses.operator-=(newZeroedClauses);

}

/**
This method sets the maiximal number of variables (in vars) in the MPE assignment.
A variable is set if:
1. it is unaffecting - contained in only zeroed terms
2. Does not belong to the parent (otherwise, it means that the parent has a branhc in which this variable 
has affect)
ASSUMPTION: unassigned_vars contains only unassigned vars
*/
void MPEMsgBuilder::setUnaffectingVarsMPE(const DBS& zereodTerms, const DBS& unassigned_vars, Assignment& MPEAssignment){
	const DBS& parentVars = (TN->getParent() != NULL) ? TN->getParent()->getVarsBS() : TreeNode::emptyDBS;		
	for(size_t v = unassigned_vars.find_first() ; v != DBS::npos ; v = unassigned_vars.find_next(v)){
		//if v belongs to the parent then either it is affecting OR
		//it affects the parent but not the current tree. --> It cannot be set in this subtree, only ignored
		if(parentVars.none() || !parentVars[v]){ //v is not part of the parent -- can be set!
			if(allVarClausesZero((varType)v,zereodTerms)){ 
				//can set the variable
				varType var_v = (varType) v;
				/*
				This means that:
				1. v is irrelevant to the current subformula
				2. v does not belong to the parent --> it cannot belong to an of TN's siblings
				--> v is irrelevant in this subformula && does not appear in any siblong subformula -->
				we can assign it the optimal value.
				irrelevantVars[v] = false; //only mark as 
				*/
				probType vPos = FM.getLitProb(var_v);
				probType vNeg = FM.getLitProb(-var_v);
				if(compareLitProbs(vNeg,vPos)){							
					MPEAssignment.setVar(var_v,false);
				}
				else{					
					MPEAssignment.setVar(var_v,true);
				}			
			}

		}
	}
}


probType MPEMsgBuilder::buildMsg(Assignment& context,probType bound, DBS& zeroedClauses, Assignment* MPEAss){	
	if(TN->isDNFTerm()){
		#ifdef _DEBUG
			assert(TN->getVarsBS().is_subset_of(TN->getParent()->getVarsBS()));
		#endif
		if(zeroedClauses[TN->getId()])
			return CombinedWeight::baseVal();
		return getDNFTermMPProb(context,MPEAss);		
	}
	DirectedGraph* relevantOrderGraph = NULL;
	//look for the MPE val in cache
	size_t cacheKey = 0;
	MPECacheStorage* cacheStore = NULL;
	cacheKey = Cache->getKey(context,zeroedClauses);	
	cacheStore = Cache->get(context, zeroedClauses, cacheKey);
	if(cacheStore != NULL){		
		if(MPEAss != NULL){
			MPEAss->setOtherAssignment(*cacheStore->MPEAss);
		}
		return cacheStore->p;
	}
	cacheStore = upperBoundCache->get(context, zeroedClauses, cacheKey);
	if(cacheStore != NULL){			
		if(!compareAssignmentProbs(cacheStore->p, bound)){ //no point in redoing the computation since the upperbound is below the limit		
			return cacheStore->p;
		}
		else{
			relevantOrderGraph=cacheStore->DOG;
			upperBoundCache->deleteEntry(context,zeroedClauses,cacheKey);			
		}
	}
	//set the node level for backtracking	
	if(Params::instance().CDCL){ 
		updateNodeLevel();
	}		
	Assignment* irrelevantVarsMPEAssignment = new Assignment();
	probType unaffectingMPEWeight;
	//if the subformula is SAT (UNSAT) then place the subtree MPE assignment in cache and return
	const DBS& subtreeTerms = TN->getDNFIds();
	
	if(subtreeTerms.is_subset_of(zeroedClauses)){
		DBS unassignedSubtreeVars = TN->getSubtreeVars();
		unassignedSubtreeVars.operator-=(context.getAssignedVars());
		if(TN->getParent() != NULL){
			unassignedSubtreeVars.operator-=(TN->getParent()->getVarsBS());
		}
		if(Params::instance().ibound > 0){
			delete irrelevantVarsMPEAssignment;
			irrelevantVarsMPEAssignment=NULL;
		}
		unaffectingMPEWeight = OptimalAssignmentProb(unassignedSubtreeVars,irrelevantVarsMPEAssignment);
		//unaffectingMPEWeight = irrelevantVarsMPEAssignment->getAssignmentProb().getVal();		
		//in this case calculate the MPE assignment for the complete subtree
		//and return	
		MPECacheStorage* cacheStore = new MPECacheStorage(unaffectingMPEWeight,irrelevantVarsMPEAssignment,NULL);	
		Cache->set(cacheStore,context,zeroedClauses,cacheKey);		
		if(MPEAss != NULL && irrelevantVarsMPEAssignment!= NULL)
			MPEAss->setOtherAssignment(*irrelevantVarsMPEAssignment);
		//assert(irrelevantVarsMPEAssignment->getAssignedVars().intersects(context.getAssignedVars())==false);		
		return unaffectingMPEWeight; //no point in processing children or assignments!!

	}
	//we need to procewss children anyway --> set only local node vars
	DBS unassignedNodeVars = TN->getVarsBS();
	unassignedNodeVars.operator-=(context.getAssignedVars());
	setUnaffectingVarsMPE(zeroedClauses,unassignedNodeVars,*irrelevantVarsMPEAssignment);
	unassignedNodeVars.operator-=(irrelevantVarsMPEAssignment->getAssignedVars());		
	#ifdef _DEBUG
	assert(MPEAss == NULL || !irrelevantVarsMPEAssignment->getAssignedVars().intersects(MPEAss->getAssignedVars()));
	#endif

	//build formula only based on relevant DNF nodes (those that were not zeroed out by the parent assignment)	
	//the MPE assignement is considered part of the "new assignments" generated by this node.
	updateGlobalAssignment(context);

	assert(context.getAssignedVars().intersects(irrelevantVarsMPEAssignment->getAssignedVars())==false);

	//new, updated context - only for retrieving the relevant members
	if(relevantOrderGraph == NULL){ //only if it has not been previously calculated
		context.setOtherAssignment(*irrelevantVarsMPEAssignment);	
		 varSet relevantVars;		
		 getRelevantMembers(context,relevantVars,zeroedClauses);	
		 relevantOrderGraph = getRelevantGraphNew(relevantVars,zeroedClauses);
		context.clearOtherAssignment(*irrelevantVarsMPEAssignment);	
	}
		
	CombinedWeight p;
	CombinedWeight boundCalc;	
	//probType retVal = worstRetVal(); //initialize to worst value.
	probType retVal = worstRetVal(bound); //initialize to worst value.
	
	configuration_iterator endIt = configuration_iterator::end(*TN);	
	const list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();
	int numOfConfigs = 0;
	configuration_iterator configAt = configuration_iterator::begin(relevantOrderGraph,p.getVal(),*TN,bound);	

	DBS newZeroedClauses;
	newZeroedClauses.resize(FM.getNumTerms(),false);	
	//the set of vars that are currently relevant	
	//DBS contextRelevantVars = TN->getVarsBS();
	//Utils::fromSetToDBS(contextRelevantVars,relevantVars);

	probType currBound = bound; //initial lower/upper bound as recieved from parent		
	DBS unassignedCurrAssVars = unassignedNodeVars;	
	Assignment* MPEAssignment= (Params::instance().ibound==0 ? new Assignment() : NULL); //to cache	
	for(; configAt != endIt ; ++configAt){		
		numOfConfigs++;				
		Assignment currAss((*configAt).A);						
		currAss.setOtherAssignment(*irrelevantVarsMPEAssignment); //current assignment to local unassigned relevant vars		
		CombinedWeight tcfAssProb=currAss.getAssignmentProb(); //the probability corresponding to currAss+irrelevantVarsMPEAssignment
		if(!compareAssignmentProbs(tcfAssProb.getVal(),currBound)){			
			continue;
		}
		
		//update set of zereod clauses
		newZeroedClauses.reset();
		calculateZeroedClauses(currAss,newZeroedClauses);
		newZeroedClauses.operator-=(zeroedClauses); //now contains only the zereod clauses following the currAss	
		zeroedClauses.operator|=(newZeroedClauses);	

		unassignedCurrAssVars.operator|=(unassignedNodeVars);
		unassignedCurrAssVars.operator-=(currAss.getAssignedVars());		
		//now contains vars that appear in 0 subtrees - that were previously considered relevant	
		setUnaffectingVarsMPE(zeroedClauses,unassignedCurrAssVars,currAss);						
		tcfAssProb.setVal(currAss.getAssignmentProb().getVal());		
		
		if(!compareAssignmentProbs(tcfAssProb.getVal(),currBound)){ //do not bother calculating the children --> will not pass the bound anyway		
			//save the highest configuration
			if(compareAssignmentProbs(tcfAssProb.getVal(),retVal)){			
				retVal = tcfAssProb.getVal();				
			}
			zeroedClauses.operator-=(newZeroedClauses);//ok , the two sets are disjoint	
			continue;
		}
		context.setOtherAssignment(currAss);		
		//check if this assignment is even possible		
		
		//probType maxSubFormProb = CombinedWeight::baseVal();		//childrenUpperBounds(context);						
		//the maximal probability of this entry		
		
		unassignedCurrAssVars.operator-=(context.getAssignedVars());		
		probType currVarsUP = OptimalAssignmentProb(unassignedCurrAssVars);
		
		boundCalc.setVal(tcfAssProb.getVal());
	//	boundCalc*=maxSubFormProb;
		boundCalc*=currVarsUP;		
		if(!compareAssignmentProbs(boundCalc.getVal(), currBound)){ //in the best case..					
			//save the highest configuration
			if(compareAssignmentProbs(boundCalc.getVal(),retVal)){			
				retVal = boundCalc.getVal();				
			}
			context.clearOtherAssignment(currAss);	
			zeroedClauses.operator-=(newZeroedClauses);//ok , the two sets are disjoint		
			continue; //no point in processing an assignment whose probability is less that the required lower bound	
		}
		
		
				
		factorSize++;
		totalNumOfEntries++;
		probType entryProb = tcfAssProb.getVal();			
		CombinedWeight childrenResultsProb;						
		Assignment contextSetAss(currAss);			
		for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){			
			TreeNode* currChild = *child;							
			probType childBound;
			if(Params::instance().ADD_LIT_WEIGHTS){
				childBound = currBound - (entryProb+childrenResultsProb.getVal());
			}
			else{
				if(FM.get_allVarProbsLessThan1()){ //we can assume that each child will return a probability <= 1.0
					//--> the childBound has to be at least the mana 
					childBound = currBound/(entryProb*childrenResultsProb.getVal());								
				}
				else{
					childBound=0.0;//cannot assume anything
				}			
			}		
			Assignment* MPEAssforChild=(Params::instance().ibound > 0 ? NULL : &currAss);
			probType childProb = currChild->buildMessage(context,childBound,zeroedClauses,MPEAssforChild);						
			childrenResultsProb*=childProb;			
			tcfAssProb *= childProb;
			if(!compareAssignmentProbs(tcfAssProb.getVal(),currBound)){			
				break; //no point in continuing											
			}						
		}		

		context.clearOtherAssignment(contextSetAss);	
		zeroedClauses.operator-=(newZeroedClauses);//ok , the two sets are disjoint		

		//Now that currAss has been updated by the children, make sure that all irrelevant node vars have been set
		//A var that has remained unset, is one which should have been set by one of the children
		//if it has not been set it means that all children thought it would be set by a subling --> parent should set it.
		if(compareAssignmentProbs(tcfAssProb.getVal(), currBound)){			
			unassignedCurrAssVars.operator-=(currAss.getAssignedVars());	
			unassignedCurrAssVars.operator-=(context.getAssignedVars());
			if(TN->getParent() != NULL){
				unassignedCurrAssVars.operator-=(TN->getParent()->getVarsBS());
			}
			if(!unassignedCurrAssVars.none()){
				OptimalAssignmentProb(unassignedCurrAssVars,&currAss);			
			}
		}

		//the parent assignment requires BT
		if(!compareAssignmentProbs(tcfAssProb.getVal(),currBound) && 
				BacktrackSM::getInstance()->BacktrackRequired(context)){ //current assignment falsifies the learned clause									
			break;			
		}	

		//save the highest configuration
		if(compareAssignmentProbs(tcfAssProb.getVal(), retVal)){		
			retVal = tcfAssProb.getVal();			
			if(compareAssignmentProbs(retVal,currBound)){//in this case tcfAssProb > currBound -->currAss must point to a valid assignemnt
				currBound=retVal;
				configAt.setBound(currBound); //improve the lower bound	
				if(MPEAssignment != NULL){
					MPEAssignment->clearAssignment();
					MPEAssignment->setOtherAssignment(currAss);				
				}
			}						
		}
	}

	//if retVal = 0, it means that none of the configuration withstood the currLB bound	
	if(compareAssignmentProbs(retVal,bound)){// found at least one confiuration above the lower bound
		cacheStore = new MPECacheStorage(retVal,MPEAssignment,relevantOrderGraph);	
		Cache->set(cacheStore,context,zeroedClauses,cacheKey);		
		if(MPEAss != NULL && MPEAssignment != NULL)
			MPEAss->setOtherAssignment(*MPEAssignment);
	}
	else{
		cacheStore = new MPECacheStorage(retVal,NULL,relevantOrderGraph);	
		upperBoundCache->set(cacheStore,context,zeroedClauses,cacheKey);		
		if(MPEAssignment != NULL)
			delete MPEAssignment;
	}
	delete irrelevantVarsMPEAssignment;
	return retVal;

}


//calculates the upper bound for each child
probType MPEMsgBuilder::childrenUpperBounds(const Assignment& context){
	size_t idx = 0;
	CombinedWeight retVal;
	const list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();			
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){	
		assignedChildVars.reset();
		unassignedChildVars.reset();		
		TreeNode* currChild = *child;
		unassignedChildVars.operator|=(currChild->getSubtreeVars());
		unassignedChildVars.operator-=(TN->getVarsBS());
		unassignedChildVars.operator-=(FM.getWeight1Vars());
		//at this point unassignedChildVars contains all subtree vars whose weight != 0/1
		//and that belong solely to the child subtree.
		assignedChildVars.operator|=(unassignedChildVars);
		unassignedChildVars.operator-=(context.getAssignedVars()); //remove assigned vars
		assignedChildVars.operator&=(context.getAssignedVars()); //common set
#ifdef _DEBUG
		DBS test(unassignedChildVars);
		test.operator|=(assignedChildVars);
		DBS onlyChildVars(currChild->getSubtreeVars());
		onlyChildVars.operator-=(TN->getVarsBS());
		onlyChildVars.operator-=(FM.getWeight1Vars());
		assert(test==onlyChildVars);
#endif
		if(true || unassignedChildVars.count() <=  assignedChildVars.count()){
			//calulate by setting value to unassigned vars
			probType child_up = OptimalAssignmentProb(unassignedChildVars);					
			currChild->upperBound = child_up;
		}
		else{ //assignedChildVars.count() < unassignedChildVars.count()
			#ifdef _DEBUG
			probType regComp=OptimalAssignmentProb(unassignedChildVars);
			assert(currChild->genericUP >=0);
			#endif			
			CombinedWeight childUP(currChild->genericUP);
			//var values to divide by - must have at least one assigned var			
			if(!assignedChildVars.none()){
				for(size_t v=assignedChildVars.find_first() ; v != boost::dynamic_bitset<>::npos ; 
											v = assignedChildVars.find_next(v)){
					probType litProb = bestProbForVar((varType)v);				
					childUP.operator/=(litProb);
				}
			}
			#ifdef _DEBUG				
				assert(Utils::essentiallyEqual(childUP.getVal(),regComp));				
			#endif
			currChild->upperBound=childUP.getVal();
		}
		retVal.operator*=(currChild->upperBound);
	}
	
	return retVal.getVal();
}

void MPEMsgBuilder::calculategenericUpperBounds(TreeNode* node){	
	//assign the non-parent vars
	DBS nonParentVars=node->getVarsBS();
	nonParentVars.operator-=(FM.getWeight1Vars());
	if(node->getParent() != NULL){
		nonParentVars.operator-=(node->getParent()->getVarsBS());
	}
	probType currNodeUP = OptimalAssignmentProb(nonParentVars);
	CombinedWeight UPCW(currNodeUP);
	const list<TreeNode*>& children = node->getChildren();
	list<TreeNode*>::const_iterator childEnd = children.end();	
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != childEnd ; ++child){
		TreeNode* currChild=*child;
		calculategenericUpperBounds(currChild);
		UPCW.operator*=(currChild->genericUP);
	}
	node->genericUP=UPCW.getVal();
	#ifdef _DEBUG						
		DBS unassignedChildVars=(node->getSubtreeVars());
		if(node->getParent() != NULL){
			unassignedChildVars.operator-=(node->getParent()->getVarsBS());
		}
		unassignedChildVars.operator-=(FM.getWeight1Vars());
		probType bestProb=OptimalAssignmentProb(unassignedChildVars);
		assert(Utils::essentiallyEqual(bestProb,node->genericUP));
	#endif
}


probType MPEMsgBuilder::getDNFTermMPProb(const Assignment& context,Assignment* MPEAss) const{	
	CombinedWeight MPEProb;
	if(!TN->isDNFTerm()){ //not  DNF term,cannot be satisfied		
		MPEProb.getVal();
	}	
	/*check if term is zeroed
	If term is zeroed then the unset variables must belong to a sibling clause and therefore cannot be set (that is
	the reason they were not set by the parent)
	If vclause is not zeroed and has an un set var then we know that it is not part of any othe sibling
	*/
	const varSet& vars = TN->getVars();
	varSet::const_iterator end = vars.end();	
/*	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		varType v = *it;
		Status vStat = context.getStatus(v);
		if(vStat == SET_F){ //a zeroed clause			
			return MPEProb.baseVal(); // may be 1 or 0 dependeing on whether we are in ADD or MULT mode			
		}
	}	*/
	
	varType varToSet=0;		
	//probType MPEProb = 1.0;	
	probType maxZeroOneRatio = worstZeroOneRatio() ; //i.e., MAX[p(l^)/p(l)] in mult mode or MIN[p(l^)-p(l)] if in ADD mode		
	bool isZeroed = false;
	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		varType v = *it;
		Status vStat = context.getStatus(v);
		if(vStat != UNSET){	//if here then the term is not zeroed --> cannot be turned zeroed		
			continue;
		}	
		//if here, v is an unassigned MPE variable in an unzeroed clause --> all siblings containing v MUST be zeroed -->
		//safe to set
		probType posProb = FM.getLitProb(v);
		probType negProb = FM.getLitProb(0-v);
		if(compareLitProbs(negProb,posProb)){//better for the literal to be assigned 0		
			MPEProb*=negProb;
			isZeroed = true;	
			if(MPEAss != NULL){
				MPEAss->setVar(v,false);
			}
		}
		else{ //here  posProb > 0
			MPEProb*=posProb;
			//probType ratio = (ADD_VARS ? negProb-posProb : negProb/posProb); //in case all MAP vars are assigned 1
			probType ratio = computeRatio(negProb,posProb);
			if(compareRatios(ratio,maxZeroOneRatio)){			
				maxZeroOneRatio = ratio;
				varToSet = v;
			}
			if(MPEAss != NULL){
				MPEAss->setVar(v,true);
			}			
		}
	
	}	
	//DEBUG
	//string ctxtStr = context.print();
	
	if(isZeroed){
		return MPEProb.getVal(); //the MPE assignment already zero this term --> return max probability
	}
	if(MPEAss != NULL && varToSet != 0){
		MPEAss->setVar(varToSet,false);
	}
	
	return((MPEProb*=maxZeroOneRatio).getVal());
}

void MPEMsgBuilder::initByTN(){	
	Cache = new MPECache(TN);
	upperBoundCache = new MPECache(TN);	
//	calculategenericUpperBounds(TN); - remove because i stopped using it!!
}

probType MPEMsgBuilder::OptimalAssignmentProb(const DBS& unassignedVars,Assignment* optimalAss) const{
	//probType retval = 1.0;
	CombinedWeight retval;
	if(unassignedVars.is_subset_of(FM.getWeight1Vars())){
		if(optimalAss != NULL){
			optimalAss->setAllToFalse(unassignedVars);
		}
		return retval.getVal();
	}

	for(size_t v=unassignedVars.find_first() ; v != boost::dynamic_bitset<>::npos ; v = unassignedVars.find_next(v)){				
		varType var_v = (varType)v;
		probType posProb = FM.getLitProb(var_v);
		probType negProb = FM.getLitProb(-var_v);
		//retval*=(posProb > negProb) ? posProb : negProb;
		bool negBetter = compareLitProbs(negProb,posProb) ;
		retval*= (negBetter ? negProb:posProb);
		if(optimalAss != NULL){
			optimalAss->setVar(var_v, (negBetter ? false : true));			
		}
	}
	return retval.getVal();
}


void MPEAssignmentCache::set(MPECacheStorage* MPEResult, const Assignment& ga,  const DBS& zeroedTerms, size_t cacheKey){
	//DEBUG:
	//string setStr = ga.print();

	relevantAssignedVars.operator|=(varsBS); //make sure this DBS contains the relevant vars
	relevantAssignedVars.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	DBSToMPEStore& cache = maskToCache[relevantAssignedVars]; //get appropriate cache entry
	
	//now get the assignment key for the hash
	relevantAssignedVars.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	

	cache[relevantAssignedVars]= MPEResult; //place in the cache		
}
	
MPECacheStorage* MPEAssignmentCache::get(const Assignment& ga, const DBS& zeroedTerms, size_t cacheKey){
	//DEBUG:
	//string getStr = ga.print();

	relevantAssignedVars.operator|=(varsBS); //make sure this DBS contains the relevant vars
	relevantAssignedVars.operator&=(ga.getAssignedVars()); //vars that are both relevant and assigned
	DBSToMPEStore& cache = maskToCache[relevantAssignedVars]; //get appropriate cache entry

	//now get the assignment key for the hash	
	relevantAssignedVars.operator&=(ga.getAssignment()); //get the assignment cooresponding to the relevant and assigned vars	
	DBSToMPEStore::const_iterator it = cache.find(relevantAssignedVars);
	if(it == cache.cend()){
		return NULL;
	}
	return (it->second);	
}


void MPEMsgBuilder::clearCacheSaveUP(){
	upperBoundCache->addAll(Cache);
	Cache->clearAll();		
	const std::list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator end = children.end();
	//recursively call children
	for(list<TreeNode*>::const_iterator it = children.begin() ; it != end ; ++it){
		TreeNode* child = *it;
		MPEMsgBuilder* MPE_builder=	(MPEMsgBuilder*)child->getMSGGenerator();
		MPE_builder->clearCacheSaveUP();
	}
}