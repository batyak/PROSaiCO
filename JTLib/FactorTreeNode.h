#ifndef FACTOR_TREE_NODE_H
#define FACTOR_TREE_NODE_H

#include <boost/unordered_map.hpp>
#include "Definitions.h"
#include "Assignment.h"
#include "DirectedGraph.h"
#include "Utils.h"
#include "FormulaMgr.h"

class FactorTreeNode{
public:
	FactorTreeNode(const Assignment& ass){
		implied = new Assignment(ass);
		prob = ass.getAssignmentProb().getVal();		
		low=high=parent = NULL;
		rightChildSet = leftChildSet = false;	
		rootDistance = (parent == NULL ? 0 : parent->rootDistance+1);
	}
	FactorTreeNode(probType p){
		implied = NULL;
		prob = p;		
		low=high=parent = NULL;
		rightChildSet = leftChildSet = false;	
		rootDistance = (parent == NULL ? 0 : parent->rootDistance+1);
	}

	void setRightChild(FactorTreeNode* rightChild){
		this->high = rightChild;
		if(rightChild != NULL){
			rightChild->parent = this;
			rightChild->rootDistance=rootDistance+1;
		}		
		rightChildSet = true;		
	}

	void setLeftChild(FactorTreeNode* leftChild){
		this->low = leftChild;		
		if(leftChild != NULL){
			leftChild->parent = this;
			leftChild->rootDistance=rootDistance+1;
		}		
		leftChildSet = true;		
	}

	
	virtual ~FactorTreeNode(){		
		if(leftChildSet && low != NULL){
			delete low;
			low = NULL;
		}
		if(rightChildSet && high != NULL){
			delete high;
			high = NULL;
		}
		if(implied != NULL){
			delete implied;
		}
	/*	if(emptyAss != NULL){
			delete emptyAss;
			emptyAss = NULL;
		}*/

	}
	const Assignment& getImplied() const{
		if(implied != NULL){
			return *implied;
		}
		if(emptyAss == NULL){
			emptyAss = new Assignment();
		}
		return *emptyAss;
	}

	probType prob; //this is actually the probability conditioned on the ancestors of the node.	
	FactorTreeNode* parent;		
	FactorTreeNode* low;
	FactorTreeNode* high;
	bool rightChildSet;
	bool leftChildSet;
	static DBS emptyDBS;
	static bool minCostSAT;
	virtual bool isLeaf(){
		return false;
	}
	virtual bool getBounded() const{
		return false;
	}

protected:
	void setImplied(const Assignment& nimplied){
		implied->clearAssignment();
		implied->setOtherAssignment(nimplied);
	}

	size_t rootDistance;	
private:
	Assignment* implied;	
	static Assignment* emptyAss;
};

class FactorTreeLeaf:public FactorTreeNode{
public:
	FactorTreeLeaf(const Assignment& ass):FactorTreeNode(ass),bounded(false){}
	FactorTreeLeaf(probType p):FactorTreeNode(p),bounded(false){}
	bool isLeaf(){
		return true;
	}	
	bool getBounded() const{
		return bounded;
	}
	void setBounded(bool bounded){
		this->bounded = bounded;
	}
private:
	bool bounded;
};

class FactorTreeInternal:public FactorTreeNode{
private:	
	struct varScore{
		size_t commonVars;
		size_t inDeg;		
		bool isDeterminedVar; //non-indicator vars that are determined by the rest
		probType MPEScore;		
	};
	
	static DBS vInBS;
	static DBS v_InBS;
	static DBS vTotalIn;
	static void setUpScoreCompDBS(const DirectedGraph& nodeOCG);
	
	static dlevel getNextAsssignmentLevel();

	//assumption - v is part of the OCG
	void getVarScore(varType v, varScore& retVal){		
		vInBS.reset();
		v_InBS.reset();
		vTotalIn.reset();
		retVal.inDeg=0;
		retVal.commonVars=0;
		retVal.MPEScore = 0;
		v = (v > 0) ? v : 0-v;		
		vInBS.operator|=(nodeOCG.getIncomingBS(v));		
		vInBS.operator&=(relevantVars);		
		v_InBS.operator|=(nodeOCG.getIncomingBS(0-v));
		v_InBS.operator&=(relevantVars);		
		vTotalIn.operator|=(vInBS);		
		vTotalIn.operator|=(v_InBS);
		retVal.inDeg = vTotalIn.count();
		vInBS.operator&=(v_InBS);
		retVal.commonVars = vInBS.count();	
		FormulaMgr* FM = FormulaMgr::getInstance();
		retVal.isDeterminedVar = FM->isDeterminedVar(v);		
		if(FM->isMPVar(v)){
			retVal.MPEScore = FM->getLitProb(v);
		}
	}

	//returns true if s1 < s2
	bool compareScores(const varScore& s1, const varScore& s2) const{		
		if(s1.commonVars == s2.commonVars){ //most important criteria
			if(s1.inDeg == s2.inDeg){
				return (minCostSAT ? s1.MPEScore <= s2.MPEScore : s1.MPEScore > s2.MPEScore);				
			}
			else{
				return (s1.inDeg < s2.inDeg);
			}
		}
		else{
			return (s1.commonVars < s2.commonVars);
		}
	}

	varType getNodeVar(){
		size_t selectedVar=0;
		varScore bestVarScore;
		//select the variable with the smallest score		
		//initialize to max/worst possible values
		bestVarScore.commonVars = relevantVars.count();
		bestVarScore.inDeg = relevantVars.count();
		bestVarScore.isDeterminedVar = true;
		varScore currVarScore;
		for(size_t v_idx=relevantVars.find_first() ; v_idx != boost::dynamic_bitset<>::npos; v_idx = relevantVars.find_next(v_idx) ){
			varType v_t = nodeOCG.getVarByIdx(v_idx);	
			if(v_t < 0) continue;
			getVarScore(v_t,currVarScore);
			//vScore < bestVarScore
			if(compareScores(currVarScore,bestVarScore)){
				selectedVar = v_t;
				bestVarScore = currVarScore;
			}
		}		
		selectedVar = (selectedVar > 0) ? selectedVar : 0-selectedVar;
		return (varType)selectedVar;
	}

public:	
	
	FactorTreeInternal(const DBS& vars, DirectedGraph& OCG, const Assignment& imp): FactorTreeNode(imp),relevantVars(vars), 
							nodeOCG(OCG){
		assert(nodeOCG.getVertices().size() > 0);		
		setUpScoreCompDBS(nodeOCG);

		//DEBUG
		//string s = nodeOCG.printGraph();		
		level = getNextAsssignmentLevel();
		v = getNodeVar();
		//update graph to contain only the variables with incoming edges to either v or -v
		DBS incoming_v = nodeOCG.getIncomingBS(v);
		incoming_v.operator|=(nodeOCG.getIncomingBS(0-v));
		relevantVars.operator&=(incoming_v);		
	}

	

	//build the conditioning graph for the child node based on the given assignment
	FactorTreeNode* getNextNode(const Assignment& currAss, size_t iBound =0){
		//stop at a bound, return a leaf, regardless
		if(iBound > 0 && this->rootDistance >= iBound){
			FactorTreeLeaf* retVal = new FactorTreeLeaf(currAss);
			retVal->setBounded(true);
			return retVal;
		}
		//get the relevant variables based on the current conditioning graph and the assignment
		Status vStat = currAss.getStatus(v);
		assert(vStat != UNSET);	//must be set here

		DBS newNodeRelVars = relevantVars;
		if(vStat == SET_F){			
			newNodeRelVars.operator&=(nodeOCG.getIncomingBS(v));			
		}
		else{//vStat == SET_T			
			newNodeRelVars.operator&=(nodeOCG.getIncomingBS(0-v));			
		}
		//get the assigned vars
		DBS AssignedRelevant(newNodeRelVars.size());	
		DBS AssignedRelevantFalse(newNodeRelVars.size());
		AssignedRelevant.reset();
		AssignedRelevantFalse.reset();
		for(size_t i = 0; i < AssignedRelevant.size() ; i++){
			varType v_i = nodeOCG.getVarByIdx(i);
			Status v_iStat = currAss.getStatus(v_i);
			if(v_iStat != UNSET){ //this variable is assigned
				AssignedRelevant.set(i,true);				
			}
			if(v_iStat == SET_F){ //we look only at false because both types of lits are nodes in the graph
				AssignedRelevantFalse.set(i,true);
			}
		}	

		//Deal with assigned vars in this factor		
		if(AssignedRelevant.count() > 0){
			newNodeRelVars.operator-=(AssignedRelevant); //newNodeRelVars now contains the unassigned variables			
			//compute the literals that become irrelevant due to currAss
			DBS litsToDelete(newNodeRelVars.size()); 
			litsToDelete.reset();
			for(size_t t = newNodeRelVars.find_first() ;t != DBS::npos; t= newNodeRelVars.find_next(t) ){				
				//t is relevant at this stage because it has an edge to v(-v), and recursively to all other lits in this context
				//only chance to remove it is if one of the vars in the current graph is assigned and there is no edge
				varType v_t = nodeOCG.getVarByIdx(t);
				const DBS& out_t = nodeOCG.getOutgoingBS(v_t);
				bool assignedSubsetOfout_t = AssignedRelevantFalse.is_subset_of(out_t);
				if(!assignedSubsetOfout_t){ //there exists an assigned var v for which there is no edge between v_t-->v
					litsToDelete.set(t,true); //mark t for deletion						
				}						
			}			
			newNodeRelVars.operator-=(litsToDelete); //remove irrelevant lits
		}
	
		FactorTreeNode* retVal = NULL;
		if(newNodeRelVars.count() == 0){			
			retVal = new FactorTreeLeaf(currAss);			
		}
		else{
			//at this point we know that 
			retVal = new FactorTreeInternal(newNodeRelVars, nodeOCG,currAss);
		}
		return retVal;		
	}	
	
	bool isLeaf(){
		return false;
	}

	~FactorTreeInternal(){
		relevantVars.clear();		
	}

	varType v;	
	dlevel level; //place in the heirarchy
	static dlevel currAssignmentlevel;
	DirectedGraph& nodeOCG;
	DBS relevantVars;
	static DirectedGraph emptyGraph;


	void updateValsByChildren(){ //NULLS indicate 0 prob
		if(!rightChildSet || !leftChildSet)
			return;
		probType lowProb = (low == NULL) ? 0.0 : low->prob;
		probType highProb = (high == NULL) ? 0.0 : high->prob;
		if(FormulaMgr::getInstance()->isMPVar(v)){
			FactorTreeNode* best = (lowProb >= highProb) ? low : high;
			prob = (best == NULL ? 0.0 : best->prob);			
			if(best != NULL){
				setImplied(best->getImplied());
			}
		}
		else{
			prob=(lowProb+highProb);
		}

	}



	
};


#endif
