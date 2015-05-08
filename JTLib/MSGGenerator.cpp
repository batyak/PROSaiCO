#include "MsgGenerator.h"
#include "ContractedJunctionTree.h"

size_t MSGGenerator::totalNumOfEntries=0;
size_t MSGGenerator::cacheHits=0;
size_t MSGGenerator::numOfBTs=0;
size_t MSGGenerator::numOfPrunes=0;

MSGGenerator::MSGGenerator(TreeNode* TN_):TN(TN_),FM(ContractedJunctionTree::getInstance()->getFormulaMgr()){
	factorSize = 0;
	zeroedEntries = 0;
	compressedToReg = NULL;
	RegToCompressed = NULL;
	//map term ids for efficient ORing
	const DBS& subTerms = TN->getDNFIds();
	numOfTNSubTerms = subTerms.count();
	compressedToReg = new size_t[subTerms.count()];	//for the compressed indexes
	RegToCompressed = new size_t[subTerms.size()]; //for the regular indexes
	size_t mappedIdx = 0;
	for(size_t idx = subTerms.find_first() ; idx !=  boost::dynamic_bitset<>::npos ; idx = subTerms.find_next(idx)){
		RegToCompressed[idx]=mappedIdx;
		compressedToReg[mappedIdx]=idx;
		mappedIdx++;
	}
}

/*
DirectedGraph* MSGGenerator::getRelevantGraph(const varSet& relevantVars, const DBS& zeroedTerms){

	DirectedGraph* OCG = TN->getOrderConstraintGraph();
	DirectedGraph* subGraph = new DirectedGraph(relevantVars);
	varSet::const_iterator varsEnd = relevantVars.end();
	for(varSet::const_iterator varsIt = relevantVars.begin() ; varsIt != varsEnd ; ++varsIt){
		varType from = *varsIt;
		if(from < 0)
			continue;
		size_t fromInd = OCG->getVarIdx(from); //perform only once		
		size_t fromIndSubGraph = subGraph->getVarIdx(from); //perform only once
		size_t _fromIndSubGraph = subGraph->getVarIdx(-from); //perform only once
		for(varSet::const_iterator varsIt2 = relevantVars.begin() ; varsIt2 != varsEnd ; ++varsIt2){
			varType to = *varsIt2;
			if(from == to || from == -to){
				continue;
			}
			size_t toInd = OCG->getVarIdx(to);		
			if(OCG->isEdge(fromInd,toInd)){ //i.e., if(isEdge(from,to))
				//verify if edge is required
				if(isEdgeRequired(from,to,zeroedTerms)){
					size_t toIndSubGraph = subGraph->getVarIdx(to); //perform only once
					subGraph->addEdge(fromIndSubGraph,toIndSubGraph);
					subGraph->addEdge(_fromIndSubGraph,toIndSubGraph);
				}
			}			
		}
	}		
	return subGraph; 
}*/

DirectedGraph* MSGGenerator::getRelevantGraphNew(const varSet& relevantVars,const DBS& zeroedTerms){
	DirectedGraph* subGraph = new DirectedGraph(relevantVars);
	varSet::const_iterator varsEnd = relevantVars.end();
	DBS fromDBS; //will contain relevant from terms
	DBS calcDBS;
	fromDBS.resize(FM.getNumTerms(),false);
	calcDBS.resize(FM.getNumTerms(),false);
	for(varSet::const_iterator varsIt = relevantVars.begin() ; varsIt != varsEnd ; ++varsIt){
		varType from = *varsIt;
		if(from < 0)
			continue;
		fromDBS.reset();
		calcDBS.reset();
		fromDBS.operator|=(getTermsWithLit(from));
		fromDBS.operator|=(getTermsWithLit(-from));
		fromDBS.operator-=(zeroedTerms);
		if(fromDBS.count() < 2) //will not need any outgoing edges
			continue;
		size_t fromIndSubGraph = subGraph->getVarIdx(from); //perform only once
		size_t _fromIndSubGraph = subGraph->getVarIdx(-from); //perform only once
		for(varSet::const_iterator varsIt2 = relevantVars.begin() ; varsIt2 != varsEnd ; ++varsIt2){
			varType to = *varsIt2;
			if(from == to || from == -to){
				continue;
			}
			calcDBS.operator|=(fromDBS);
			const DBS& toLits = getTermsWithLit(to);
			calcDBS.operator-=(toLits);
			/*
			if(calcDBS.count() < 2){
				continue;
			}*/
		//	list<TreeNode*> relevantChildren;
		//	getChildrenWithVar(from,relevantChildren);
			if(!isTermsSetIsolated(calcDBS,from)){ //edge required
				size_t toIndSubGraph = subGraph->getVarIdx(to); //perform only once
				subGraph->addEdge(fromIndSubGraph,toIndSubGraph);
				subGraph->addEdge(_fromIndSubGraph,toIndSubGraph);
			}

		}

	}
	return subGraph;
}
/*
bool MSGGenerator::isEdgeRequired(varType from, varType to, const DBS& zereodTerms){
	DBS fromNonToTerms = getTermsWithLit(from);
	fromNonToTerms.operator|=(getTermsWithLit(-from)); 
	fromNonToTerms.operator-=(zereodTerms); //contains all non-zero terms with from 
	fromNonToTerms.operator&=(TN->getNonContainingTerms(to)); //contains all non-zero terms with from which do NOT contain to
	//now check if they are subsumed in a single child
	if(isTermsSetIsolated(fromNonToTerms)){
		return false;
	}
	return true;
}
*/
void MSGGenerator::updateNodeLevel(){
	TN->updateNodeLevel();
}

void MSGGenerator::updateGlobalAssignment(const Assignment& GA_){
	TN->updateGlobalAssignment(GA_);
}


void MSGGenerator::getRelevantMembers(const Assignment& GA, varSet& relevantVars, const DBS& zeroedTerms){
	DBS relevantVarsBS = TN->getVarsBS(); //start with complete set of vars
	relevantVarsBS.operator-=(GA.getAssignedVars()); //removed assigned
	//try!!
	relevantVarsBS.operator-=(FM.getDeterminedVars());
	if(relevantVarsBS.none()) return; 

	for(size_t v = relevantVarsBS.find_first() ; v != DBS::npos ; v = relevantVarsBS.find_next(v)){		
		if(!varTermsIsolated((varType)v,zeroedTerms)){ //means that the variable is relevant (appears in at least two subformulas)
			varType var = (varType)v;
			relevantVars.insert(var);
			relevantVars.insert(0-var);
		}		
	}

	//TN->getRelevantMembers(GA,relevantVars,irrelevantVars,zeroedTerms);
}

	
void MSGGenerator::translateFromCompressed(const DBS& compressed, DBS& trans) const{
	for(size_t idx = compressed.find_first() ; idx !=  boost::dynamic_bitset<>::npos ; idx = compressed.find_next(idx)){
		size_t regIdx = compressedToReg[idx];
		trans.set(regIdx,true);
	}
}

void MSGGenerator::translateToCompressed(const DBS& terms, DBS& compressed) const{
	for(size_t idx = terms.find_first() ; idx !=  boost::dynamic_bitset<>::npos ; idx = terms.find_next(idx)){
		size_t comIdx = RegToCompressed[idx];
		compressed.set(comIdx,true);
	}
}

const DBS& MSGGenerator::getCompressedTermsWithLit(varType lit){
	varToBS::iterator litIt = litToCompressedSubForm.find(lit);
	if(litIt==litToCompressedSubForm.end()){
		const DBS& regLitTerms = getTermsWithLit(lit); //regular lit terms
		DBS compressedLitTerms(numOfTNSubTerms); //create new DBS
		translateToCompressed(regLitTerms,compressedLitTerms); //fill with appropriate term ids
		litToCompressedSubForm[lit]=compressedLitTerms;
	}
	return litToCompressedSubForm.at(lit);
}

const DBS& MSGGenerator::getTermsWithLit(varType lit) {	
	varToBS::iterator litIt = litToSubForm.find(lit);
	if(litIt == litToSubForm.end()){
		DBS litTerms=TN->getDNFIds();
		litTerms.operator&=(FM.getTermsWithLit(lit));		
		litToSubForm[lit]=litTerms;		
	}
	return litToSubForm.at(lit);
}

//updates zeroed clauses in this subtree following assignment currEntry
//ASSUMPTION: assigned vars are first assigned in this node and belong to the current subtree.
void MSGGenerator::calculateZeroedClauses(const Assignment& currEntry, DBS& zeroedClauses){
	const DBS& assignedVars = currEntry.getAssignedVars();
	const DBS& assignment = currEntry.getAssignment();		
	DBS compressedZeroedClauses(numOfTNSubTerms);
	//run over all assigned vars
	for(size_t v = assignedVars.find_first() ; v != DBS::npos ; v = assignedVars.find_next(v)){		
		varType var_v = (varType)v;		
		//the literal that is zeroed following this assignment
		//if v=1, then l=-v is zero, otherwise v=0, then l=v is zeroed
		varType l = (assignment[v] ? -var_v: var_v);
		const DBS& l_DBS = getCompressedTermsWithLit(l);
		compressedZeroedClauses.operator|=(l_DBS);

		/*
		const DBS& l_DBS = MSGGenerator::getTermsWithLit(l); //zeroed out terms in subformula		
		zeroedClauses.operator|=(l_DBS);*/
	}
	translateFromCompressed(compressedZeroedClauses,zeroedClauses);
}

bool MSGGenerator::allVarClausesZero(varType v, const DBS& zeroedTerms){
	const DBS& vSubTerms = getTermsWithLit(v);	
	if(!vSubTerms.is_subset_of(zeroedTerms)){
		return false;
	}
	const DBS& _vSubTerms = getTermsWithLit(-v);
	if(!_vSubTerms.is_subset_of(zeroedTerms)){
		return false;
	}
	return true;
}


bool MSGGenerator::varTermsIsolated(varType v, const DBS& zeroedTerms){
	DBS vTerms = getTermsWithLit(v);
	vTerms.operator|=(getTermsWithLit(-v));
	vTerms.operator-=(zeroedTerms);	
	//now check if these terms are subsumed in only one of the children
	return isTermsSetIsolated(vTerms,(v > 0 ? v : -v));	
}

void MSGGenerator::getChildrenWithVar(varType v, list<TreeNode*>& vChildren) const{
	v = (v < 0 ) ? -v : v;
	const list<TreeNode*>& children = TN->getChildren();
	list<TreeNode*>::const_iterator end = children.end();
	for(list<TreeNode*>::const_iterator it = children.begin() ; it != end ; ++it){
		TreeNode* child = *it;
		const DBS& childVars = TN->getVarsBS();
		if(childVars[v]){
			vChildren.push_back(child);
		}
	}
}

/*
bool MSGGenerator::isTermsSetIsolated(const DBS& termSet, const list<TreeNode*>& relevantChildren) const{
	//now check if these terms are subsumed in only one of the children
//	const list<TreeNode*>& children = TN->getChildren();
	if(relevantChildren.size() < 2)
		return true;
	list<TreeNode*>::const_iterator end = relevantChildren.end();
	for(list<TreeNode*>::const_iterator it = relevantChildren.begin() ; it != end ; ++it){
		TreeNode* child = *it;
		const DBS& childTerms = child->getDNFIds();
		if(termSet.is_subset_of(childTerms)){
			return true;
		}
	}
	return false;
}*/

/**
	Check if the termSet, all containing v is isolated
	*/
	bool MSGGenerator::isTermsSetIsolated(const DBS& termSet, varType v) const{
		const list<TreeNode*>& children = TN->getChildren();		
		size_t fstTerm = termSet.find_first();
		if(fstTerm== DBS::npos || termSet.find_next(fstTerm) == DBS::npos){ //contains a single term
			return true;
		}
		if(children.size() <= 1 )
			return true; 
		list<TreeNode*>::const_iterator end = children.end();
		for(list<TreeNode*>::const_iterator it = children.begin() ; it != end ; ++it){
			TreeNode* child = *it;
			const DBS& childVars = child->getVarsBS();			
			if(!childVars[v]) //subtree cannot contain the terms
				continue;  
			if(child->isDNFTerm()){	//assuming this DNF term is not zeroed, and that there is another one, then we can reutn false			
				//return false;
				if(termSet[child->getId()]){ //at this point we know that |termSet|>=2--> there must be another child which contains an unzeroed clause with v
					return false;
				}
				continue;
			}
			const DBS& childTerms = child->getDNFIds();
			/*
			If here, then we know that the subtree contains terms with variable v
			There are two options:
			1. If termSet.intersects(childTerms) && !termSet.is_subset_of(childTerms)
				then wen can return false
			2. If termSet.intersects(childTerms) && termSet.is_subset_of(childTerms)
				then we can return true
			2. If !termSet.intersects(childTerms) then this subtree is irrelevant
			*/
			if(!termSet.intersects(childTerms))
				continue; 
			if(termSet.is_subset_of(childTerms)){ //terms are not limited to a single subtree
				return true;
			}		
			else{
				return false;
			}
		}
		return false;
	}