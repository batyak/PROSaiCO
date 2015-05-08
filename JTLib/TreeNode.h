#ifndef TREENODE_H
#define TREENODE_H

#include "DirectedGraph.h"
#include "Utils.h"
#include "Assignment.h"
#include <math.h>
#include <boost/ptr_container/ptr_map.hpp>
#include "Definitions.h"
#include "SimpleCache.h"
#include "Sub_Formula_IF.h"

using namespace boost;
class MSGGenerator;
class TreeNode: public SubFormulaIF{
	friend class ContractedJunctionTree;	
public:
	static varSet emptySet;
	static DBS emptyDBS;		

	TreeNode(nodeId id, const varSet& vars, bool isDNFTerm);
	
	virtual void generateDNFLeaves();
	
	const varSet& getVars() const{
		return vars;
	}

	nodeId getId() const{
		return id;
	}

	bool isDNFTerm() const {
		return isDNF;
	}
	TreeNode* getParent(){
		return parent;
	}

	const DBS& getVarsBS(){
		return varBS;
	}

	const list<TreeNode*>& getChildren() const{
		return children;
	}
	
	void addChild(TreeNode* child){		
		children.push_back(child);		
	}

	void removeChild(TreeNode* child){		
		nodeId childId = child->getId();
		list<TreeNode*>::iterator end = children.end();
		list<TreeNode*>::iterator childIt;
		for(childIt = children.begin() ; childIt != end; ++childIt){
			if((*childIt)->getId() == childId){
				break;
			}
		}
		if(childIt != end){
			children.erase(childIt);
		}
	}

	void setParent(TreeNode* parent){
		this->parent = parent;
		varSet parentVars;		
	}
	
	const string shortPrint() const;

	const string printSubtree() const;

	virtual ~TreeNode();

	virtual TreeNode* getContainingSibling(const list<TreeNode*>& siblingNodes)=0;
	void clearEdgeDirection();

	int getFactorSize(){
		return factorSize;
	}

	int getZeroedEntries(){
		return zeroedEntries;
	}

	void DEBUG_TestEdgeContainment(const DBS& uvNodes,varType u, varType v);		
	
	static size_t cacheHits;
	static size_t totalNumOfEntries;
	static size_t numOfBTs;
	static size_t totalCalls;	
	
	//MSGBuilder visitor related methods
	MSGGenerator* getMSGGenerator() const{
		return msgGenerator;
	}
	probType upperBound;
	probType genericUP; //is constant, takes into account all subtree vars
	void setMsgGenerator(MSGGenerator*);	
	probType buildMessage(Assignment& context, probType lb,DBS& zeroedClauses, Assignment* bestAssignment=0);	
	
private:
	nodeId id;
	DBS varBS;	
	bool isDNF;			
	
	/*
	returns true if this node is the LCA of node ids n1 and n2
	*/
	bool isLCA2(nodeId n1, nodeId n2,varType u, varType v);

	/*
	This method recieves a set of msg node ids, uvNodeIds, that contains ids 
	of messages that contain u but do not contain v
	if there exist two such messages for which this node is their LCA then add an edge between them
	*/
	void addEdgesIfLCA(nodeIdSet& uvNodeIds, varType u, varType v);

	/*
	This method recieves a set of msg node ids, uvNodeIds, that contains ids 
	of messages that contain u but do not contain v
	if there exist two such messages for which this node is their LCA then it returns true signaling that and edge u --> v
	needs to be added
	*/	
	bool addEdgeIfLCA(const DBS& uvNodeIds, varType u, varType v);

	void generateSubFormulas();
	
	void clearFactorCache();	
	
	bool containsVar(varType v) const;
	//returns true if this node has descendent msg nodes containing v	
	bool hasTermsContaining(varType v);			
	void getAllContainingTerms(varType u, DBS& retVal); 

	Cache* cache;	
	int factorSize; //num of distinct entries in parent relevant to this node
	int zeroedEntries;

	static void updateWithAllLits(varSet& vars);		
	bool isCacheable(const Assignment& GA);	
	void DBGTestSubTreeVars();

protected:	
	varSet vars;	
	varToTreeNodeMap varToContainingTerms;
	varToTreeNodeMap varToNonContainingTerms;	

	TreeNode* parent;	
	list<TreeNode*> children;
	
	virtual probType DNFProb(const Assignment& currAss)=0;
		
	/*
	We add an edge u-->v if there exist 2 or more messages S1,S2 in the node's subtree
	such that:
	1. S1,S2 \in varToContainingMsgs[u] \cap varToNonContainingMsgs[v] 
	2. this node is their Least Common Ancestor. (Otherwise: they are either on the same path
	or they both belong to the subtree of one of this node's children)			
	*/		
	void getReachableNodes(nodeIdSet& rootReachable, nodeIdSet& visited);	
	virtual void createCache();

	//MsgBuilder object
	//MsgBuilder* msgBuilder;
	MSGGenerator* msgGenerator;
};


struct tn_equal_to
    : std::binary_function<TreeNode*, TreeNode*, bool>
{
    bool operator()(const TreeNode* nodeA, const TreeNode* nodeB) const
    {		
		return ((nodeA->getVars()) == (nodeB->getVars()));        
    }
};

struct tn_hash
    : std::unary_function<std::string, std::size_t>
{
    std::size_t operator()(const TreeNode* node) const
    {
        size_t seed = 0;
		varSet::iterator theEnd = node->getVars().end();
		for(varSet::iterator it = node->getVars().begin(); it != theEnd; ++it){
			seed ^= boost::hash_value(*it);			
		}                

        return seed;
    }
};

typedef boost::unordered_set<TreeNode*,tn_hash,tn_equal_to> treeNodeSet;



#endif
