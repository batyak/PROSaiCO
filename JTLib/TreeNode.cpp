#include "TreeNode.h"
#include "MsgNode.h"
#include "ContractedJunctionTree.h"
#include "Utils.h"
#include "DirectedGraph.h"
#include "CTerm.h"
#include "Params.h"
#include <time.h>
#include "BacktrackSM.h"
#include "ConfigurationIterator.h"
#include "MsgGenerator.h"

using namespace boost;

size_t TreeNode::cacheHits=0;
size_t TreeNode::totalNumOfEntries=0;
size_t TreeNode::numOfBTs=0;
size_t TreeNode::totalCalls = 0;
DBS TreeNode::emptyDBS;

TreeNode::TreeNode(nodeId id,const varSet& vars, bool isDNFTerm): 
			SubFormulaIF(*FormulaMgr::getInstance(),isDNFTerm),id(id),vars(vars),isDNF(isDNFTerm){															
	varSet allSignedVars(vars);
	varSet::const_iterator varsEnd = vars.end();
	for(varSet::const_iterator varsIt = vars.begin() ; varsIt != varsEnd ; ++varsIt){
		varType negV = (0-*varsIt);
		allSignedVars.insert(negV);
	}	
	setParent(NULL);

	int numOfVars = FormulaMgr::getInstance()->getNumVars();											
	//build and clear the bitsets
	varBS.resize(numOfVars+1);							
	varBS.reset();	
	for(varSet::const_iterator it = vars.cbegin() ; it != vars.cend() ; ++it){
		varType v_ = *it;		
		varType v= (v_ > 0) ? v_ : (0-v_);		
		varBS[v]=true;
	}
	factorSize =0;
	zeroedEntries = 0;	
	cache = NULL;
	genericUP=-1;
	upperBound=CombinedWeight::baseVal();
}


//returns true if either v or -v is in vars
bool TreeNode::containsVar(varType v) const{
	varSet::const_iterator end = vars.cend();
	return (vars.find(v) != end);
}

//returns true if this node has descendent DNF term containing v
/*
bool TreeNode::hasTermsContaining(varType v){
	return (!varToContainingTermsBS[v].none());	
}

void TreeNode::initVarToTermsMaps(){

	varToContainingTermsBS.clear();
	varToNonContainingTermsBS.clear();
	varSet::const_iterator end = vars.cend();
	for(varSet::const_iterator vit = vars.cbegin() ; vit != end ; ++vit){
		varType v = *vit;
		varToContainingTermsBS[v].resize(FM.getNumTerms());		//subterms that contains v
		varToNonContainingTermsBS[v].resize(FM.getNumTerms());  //subterms that do not contain v
		varToContainingTermsBS[0-v].resize(FM.getNumTerms());	//subterms that do contain -v
		varToNonContainingTermsBS[0-v].resize(FM.getNumTerms()); //subterms that do not contain -v
	}

	const DBS& descendentDNFTerms = SubFormulaIF::getDNFIds();	

	if(descendentDNFTerms.count() == 1){ //this subtree contains exactly one DNF term
		nodeId singleTermId = descendentDNFTerms.find_first();			
		CTerm& singleTerm = FM.getTerm(singleTermId);
		varSet singleTermLits;
		singleTerm.getDNFLitSet(singleTermLits);
		for(varSet::const_iterator vit = vars.cbegin() ; vit != end ; ++vit){
			varType lit = *vit;
			if(singleTermLits.find(lit) != singleTermLits.end()){ //term contains the literal			
				varToContainingTermsBS[lit].set(singleTermId,true);	
				varToNonContainingTermsBS[0-lit].set(singleTermId,true); //a term cannot contain a literal and its negation
			}
			if(singleTermLits.find(0-lit) != singleTermLits.end()){ //term contains the literal			
				varToContainingTermsBS[0-lit].set(singleTermId,true);	
				varToNonContainingTermsBS[lit].set(singleTermId,true); //a term cannot contain a literal and its negation
			}

		}
		return;
	}
	if(SubFormulaIF::getDNFIds().none()) //this subtree does not contain any terms, simply return
		return; 
	
	//if here, subtree contains at least 2 DNF terms
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != children.end() ; ++cit){
		TreeNode* child = *cit;
		child->initVarToTermsMaps();	
		varToBS& currChildContainingMap = child->getVarToContainingTerms();
		varToBS& currChildNonContainingMap = child->getVarToNonContainingTerms();	
		for(varSet::const_iterator vit = vars.cbegin() ; vit != end ; ++vit){
			varType v = *vit;
			if(child->hasTermsContaining(v)){//child contains v			
				varToContainingTermsBS[v].operator|=(currChildContainingMap[v]);
				varToNonContainingTermsBS[v].operator|=(currChildNonContainingMap[v]);				
			}
			else{//none of the terms in the child's subtree contain v								
				varToNonContainingTermsBS[v].operator|=(child->getDNFIds());
			}
			if(child->hasTermsContaining(0-v)){ //child contains -v
				varToContainingTermsBS[0-v].operator|=(currChildContainingMap[0-v]);
				varToNonContainingTermsBS[0-v].operator|=(currChildNonContainingMap[0-v]);				
			}
			else{				
				varToNonContainingTermsBS[0-v].operator|=(child->getDNFIds());
			}
		}
	}	
}
*/
void TreeNode::DBGTestSubTreeVars(){
	const DBS& dnfIds = this->getDNFIds();	
	for(size_t idx = dnfIds.find_first() ; idx != boost::dynamic_bitset<>::npos ; idx = dnfIds.find_next(idx)){
		CTerm& term = FM.getTerm(idx);
		const std::vector<varType> lits = term.getLits();
		for(std::vector<varType>::const_iterator it = lits.begin() ; it != lits.end(); ++it){
			varType v = *it;
			v = (v > 0) ? v : 0-v;
			if(!subtreeVars[v]){
				cout << " BUG, the var " << v << " appears in term " << idx << " but is not part of subtreeVars" << endl;
			}
		}
	}
}

void TreeNode::createCache(){
	if(Params::instance().DYNAMIC_SF){
		cache = new subFormulaCache(getDNFIds(),subtreeVars);		
	}
	else{
		cache = new assignmentCache(subtreeVars);				
	}
}

void TreeNode::generateSubFormulas(){
	if(isDNFTerm()){
		//createCache();
		return;		
	}
	clearSubFormula();
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != children.end(); ++cit){
		(*cit)->generateSubFormulas();		
	}	
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != children.end(); ++cit){
		addDNFTerms((*cit)->getDNFIds());
	}
	//init cache
	createCache();	
}

void TreeNode::getAllContainingTerms(varType u, DBS& retVal){
	retVal.reset();
	retVal.operator|=(FM.getTermsWithLit(u));
	retVal.operator|=(FM.getTermsWithLit(-u));
	retVal.operator&=(getDNFIds()); 
//	retVal.operator|=(varToContainingTermsBS[u]);
//	retVal.operator|=(varToContainingTermsBS[0-u]);
}



/*
will return true if the edge u --> v needs to be added.
This occurs if this node is their LCA OR
S1 and S2 belong to two different branches but their LCA does not contain v
*/
bool TreeNode::isLCA2(nodeId n1, nodeId n2, varType u, varType v){
	const DBS& subForm = SubFormulaIF::getDNFIds();
	if(!subForm[n1] || !subForm[n2]){
		return false;//does not contain both nodes
	}
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != children.end() ; ++child){
		const DBS& childSubForm = (*child)->getDNFIds();		
		if(childSubForm[n1] && childSubForm[n2]){
			return false; //one of its children contains both nodes (which contain u and not v)			
		}		
	}
	return true;
}


bool TreeNode::addEdgeIfLCA(const DBS& uvNodeIds, varType u, varType v){
	if(uvNodeIds.count() < 2){
		return false;
	}
	list<TreeNode*>::const_iterator childEnd = children.end();
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != childEnd ; ++cit){
		const DBS& childTerms = (*cit)->getDNFIds();
		if(uvNodeIds.is_subset_of(childTerms)){		
			return false; //one of the children contains the whole set of nodes that contain u and not v --> this node cannot be their LCA.
		}		
	}	
//	DEBUG_TestEdgeContainment(uvNodeIds,u,v);
	return true;	
}

void TreeNode::DEBUG_TestEdgeContainment(const DBS& uvNodeIds,varType u, varType v){
	for(size_t S1_id = uvNodeIds.find_first() ; S1_id != boost::dynamic_bitset<>::npos ; S1_id = uvNodeIds.find_next(S1_id)){		
		for(size_t S2_id = uvNodeIds.find_next(S1_id) ; S2_id != boost::dynamic_bitset<>::npos ; S2_id = uvNodeIds.find_next(S2_id)){		
			if(isLCA2(S1_id,S2_id,u,v)){
				CTerm& term1 = FM.getTerm(S1_id);
				varSet set1;
				Utils::convertToVarSetAbs(term1.getLits(),set1);
				varSet set2;
				CTerm& term2 = FM.getTerm(S2_id);
				Utils::convertToVarSetAbs(term2.getLits(),set2);
				if(!(Utils::contains(set1,set2) || Utils::contains(set2,set1))){
					return;					
				}
			}
		}
	}		
}

void TreeNode::updateWithAllLits(varSet& vars){
	varSet::const_iterator varsEnd = vars.end();
	for(varSet::const_iterator varsIt = vars.begin() ; varsIt != varsEnd ; ++varsIt){
		varType negV = (0-*varsIt);
		vars.insert(negV);
	}
}
void TreeNode::setMsgGenerator(MSGGenerator* gen){
	msgGenerator = gen;
}

probType TreeNode::buildMessage(Assignment& context, probType lb,DBS& zeroedClauses, Assignment* bestAssignment){
	return  msgGenerator->buildMsg(context, lb, zeroedClauses,bestAssignment);
}


bool TreeNode::isCacheable(const Assignment& GA){
	if(!Params::instance().CDCL)
		return true; //no need to check the condition
	DBS subtreeAssigned = GA.getAssignedVars(); //variables currently assigned
	subtreeAssigned = subtreeAssigned.operator&=(subtreeVars); //the subtree vars currently assigned
	if(subtreeAssigned.is_subset_of(varBS)){
		return true; //caching condition fulfilled
	}
	return false;

}


void TreeNode::getReachableNodes(nodeIdSet& rootReachable, nodeIdSet& visited){	
	ContractedJunctionTree* cjt = ContractedJunctionTree::getInstance();
	visited.insert(id); //we are visitng this node
	rootReachable.insert(id);
	const nodeIdSet& rootNbs = cjt->getNodeNbs(id);		
	for(nodeIdSet::const_iterator nbIt = rootNbs.cbegin() ; nbIt != rootNbs.cend() ; ++nbIt){		
		nodeId nbId = *nbIt;
		if(visited.find(nbId) != visited.end()){ //if already visited, do not visit again
			continue;
		}
		TreeNode* nextRoot = cjt->getNode(nbId);
		nextRoot->getReachableNodes(rootReachable, visited);		
	}
}


void TreeNode::clearEdgeDirection(){
	for(list<TreeNode*>::const_iterator child = children.begin() ; child != children.end() ; ++child){
		(*child)->clearEdgeDirection();
	}
	children.clear();
	setParent(NULL);	
}

const string TreeNode::printSubtree() const{
	std::stringstream ss;
	ss << shortPrint();
	ss << "children ids: ";
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != children.end() ; ++cit){
		ss << (*cit)->id << ",";
	}
	ss << endl;
	for(list<TreeNode*>::const_iterator cit = children.begin() ; cit != children.end() ; ++cit){
		ss << (*cit)->printSubtree();
	}
	return ss.str();
}


const string TreeNode::shortPrint() const{
	std::stringstream ss;
	ss << "node id: " << this->id << ", isDNF=" << this->isDNF << endl;
	for(size_t var=varBS.find_first(); var != boost::dynamic_bitset<>::npos; var = varBS.find_next(var) ){			
		ss << var << " ";
	}
	ss << endl;	
	return ss.str();
}

TreeNode::~TreeNode(){	
	varBS.reset();		
	if(cache != NULL){
		delete cache;
	}
}


void TreeNode::generateDNFLeaves(){
	std::list<TreeNode*>::const_iterator childEnd = children.end();
	for(std::list<TreeNode*>::const_iterator childIt = children.begin() ; childIt != childEnd ; ++childIt){
		(*childIt)->generateDNFLeaves();
	}	
}