#ifndef CONFIG_IT_H
#define CONFIG_IT_H

#include "Assignment.h"
#include "FactorTreeNode.h"
#include "Utils.h"
#include "DirectedGraph.h"
#include "Sub_Formula_IF.h"
#include "Definitions.h"
#include "BacktrackSM.h"
#include "Params.h"
using namespace boost;

class AssProb{
public:
	const Assignment& A; probType& p;	
	AssProb(const Assignment& A, probType& p):A(A),p(p){};
};

class configuration_iterator : public boost::iterator_facade<
        configuration_iterator
	  , Assignment
      , boost::forward_traversal_tag
	  , AssProb //std::pair<Assignment,prob>
    >
{

private:
	friend class boost::iterator_core_access;
	static DirectedGraph emptyGraph;
	FactorTreeNode* currNode;
	FactorTreeNode* root;
	FactorTreeNode* nextConfig;
	bool isFixedAssignment;
	DirectedGraph& OCG;
	SubFormulaIF& SubFormula;
	int numOfEntries;
	probType bound;
	size_t iBound;

	inline bool betterThanBound(probType p, probType bound){
		bool retVal = (ADD ? p <= bound : p >= bound);
		if(!retVal){
			Utils::treeCPTPrunes++;
		}
		return retVal;
		//return true;
	}

	void moveLeft(){		
		if(!(currNode->isLeaf())){			
			currNode = currNode->low;
		}
	}
	void moveRight(){
		if(!currNode->isLeaf()){			
			currNode = currNode->high;
		}
	}
	void moveUp(){
		if(currNode->parent != NULL){
			currNode = currNode->parent;			
		}
	}

	

	void deleteTree(FactorTreeNode* node){
		if(node != NULL){
			if(node->isLeaf()){
				delete node;				
			}
			else{
				deleteTree(node->low);
				deleteTree(node->high);				
				delete node;
			}
		}
	}

	void initCurrNode(){ //begin by pointing to the rightmost assignment		
		if(currNode == NULL)
			return;		
		while(!currNode->isLeaf()){
			moveRight();
		}
	}

	void initRoot(probType p=1.0){
		root = NULL;
		currNode = NULL;
		if(!OCG.isEmpty()){
			Assignment rootAss;
			DBS vars;
			vars.resize(OCG.getVertices().size(),true);
			root = new FactorTreeInternal(vars, OCG,rootAss);
		}		
		else{			
			root = new FactorTreeLeaf(p);
			numOfEntries = 1;
		}
		currNode = root;
	}
	
	FactorTreeNode* getNextConfig_i(){
		if(currNode == NULL){
			return NULL;
		}
		//needed in case the root node is a leaf
		if(currNode->isLeaf()){
			FactorTreeNode* retVal = currNode;
			currNode = NULL;
			return retVal;
		}			
		while(currNode != NULL){			
			FactorTreeInternal* internalCurrNode = dynamic_cast<FactorTreeInternal*>(currNode);	
			if(internalCurrNode->rightChildSet && internalCurrNode->leftChildSet){
				internalCurrNode->updateValsByChildren(); //agregate the values of the children
				//no need for the children any longer
				if(internalCurrNode->low != NULL){
					delete (internalCurrNode->low); 
				}
				internalCurrNode->setLeftChild(NULL);
				if(internalCurrNode->high != NULL){
					delete (internalCurrNode->high);
				}
				internalCurrNode->setRightChild(NULL);
				
				currNode = internalCurrNode->parent;	
				continue;
			}
			if(!currNode->rightChildSet){ //unvisited			
				Assignment rightAss(currNode->getImplied());
				std::queue<varType> Qr;				
				Qr.push(internalCurrNode->v); //set to 1 -- i.e; rightAss.setVar(currNode->v,true);	
				SubFormula.setDecisionVar(internalCurrNode->v, internalCurrNode->level);
				bool satisfies = SubFormula.satisfiesDescendentDNF(rightAss,Qr,bound);
				if(satisfies || (!betterThanBound(rightAss.getAssignmentProb().getVal(),bound))){							
					currNode->setRightChild(NULL);				
					continue;
				}			
				FactorTreeNode* rightChild = internalCurrNode->getNextNode(rightAss,iBound);
				if(rightChild->isLeaf()){
					currNode->setRightChild(rightChild);					
					return rightChild;
				}
				currNode->setRightChild(rightChild);							
				currNode = rightChild;			
				continue;
			}			
			if(!currNode->leftChildSet){ //unvisited
				Assignment leftAss(currNode->getImplied());
				std::queue<varType> Ql;
				Ql.push(0-internalCurrNode->v); //set to 0 -- i.e; leftAss.setVar(currNode->v,false);
				SubFormula.setDecisionVar(internalCurrNode->v,internalCurrNode->level);
				bool satisfies = SubFormula.satisfiesDescendentDNF(leftAss,Ql,bound);
				if(satisfies || (!betterThanBound(leftAss.getAssignmentProb().getVal(),bound))){					
					currNode->setLeftChild(NULL);
					continue;
				}							
				FactorTreeNode* leftChild = internalCurrNode->getNextNode(leftAss,iBound);
				if(leftChild->isLeaf()){					
					currNode->setLeftChild(leftChild);					
					return leftChild;
				}	
				currNode->setLeftChild(leftChild);				
				currNode = leftChild;				
				continue;
			}
		}
		return currNode; //must be NULL here
	}

public:
	~configuration_iterator(){		
		if(root != NULL){
			delete root;
		}
	}

	static long timeToIncrement;

	//iterator is empty - no configurations
	
	explicit configuration_iterator(SubFormulaIF& sf): OCG(emptyGraph), SubFormula(sf){
		numOfEntries = 0;
		currNode = NULL;
		nextConfig = NULL;		
		root = NULL;
		isFixedAssignment = false;
		iBound=Params::instance().ibound;
	}
	
	
	//single configuration representing the leaf node.
	
	explicit configuration_iterator(probType p,SubFormulaIF& sf, probType bound): OCG(emptyGraph), SubFormula(sf){
		numOfEntries = 0;
		initRoot(p);
		currNode = root;
		this->bound = bound;
		ADD = Params::instance().ADD_LIT_WEIGHTS;
		iBound=Params::instance().ibound;
		nextConfig = getNextConfig_i();			
		isFixedAssignment = false;
		
	}
	
	//general case
	explicit configuration_iterator(DirectedGraph& orderConstraintGraph, SubFormulaIF& sf, probType bound): 
		OCG(orderConstraintGraph),SubFormula(sf){	
			numOfEntries = 0;
			initRoot();
			currNode = root;
			this->bound = bound;
			ADD = Params::instance().ADD_LIT_WEIGHTS;
			iBound=Params::instance().ibound;
			nextConfig = getNextConfig_i();	
			isFixedAssignment = false;
			
	}

	void increment(){
		if(isFixedAssignment){
			isFixedAssignment = false;
			return; //do not change the current assignment
		}		
		nextConfig = getNextConfig_i();
	}
	
	
	
	bool equal(configuration_iterator const& other) const
    {		
		return (other.nextConfig == nextConfig);
    }

   
	probType& assProb(){		
		return nextConfig->prob;
	}

	

	AssProb dereference() const{
		AssProb AP(nextConfig->getImplied(), nextConfig->prob);
		return AP;			
	}

	AssProb getAggregateAP() const{
		AssProb AP(root->getImplied(), root->prob);
		return AP;
	}

	void setBound(probType bound){
		this->bound = bound;
	}	

	static configuration_iterator begin(DirectedGraph* relevantOCG,probType leafProb,SubFormulaIF& sf,probType bound = 0.0);
	static configuration_iterator end(SubFormulaIF& sf);
	bool ADD;
};







#endif
