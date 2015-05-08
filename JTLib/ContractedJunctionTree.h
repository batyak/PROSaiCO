#ifndef CONTRACTED_JT_H
#define CONTRACTED_JT_H

#include "TreeNode.h"
#include "MsgNode.h"
#include "CliqueNode.h"
#include "Eliminator.h"
#include "JuncTreeDll.h"
#include "Definitions.h"
#include "FormulaMgr.h"
#include <iostream>
#include <queue>

using namespace boost;


class TreeGen;
class ContractedJunctionTree{

private:	

	const nodeIdSet DNFTerms;
	static ContractedJunctionTree* instance;	
	static varSet global_probAssignedVars;
	static size_t largestClique;
	static nodeId runningIdNum;
	static nodeIdSet emptyNodeIdSet;	
	unordered_map<nodeId,CliqueNode*> cliqueNodes;
	unordered_map<nodeId,MsgNode*> msgNodes;
	unordered_map<nodeId, nodeIdSet> treeEdges;
	unordered_map<nodeId,nodeIdSet> contractedTreeEdges;
	unordered_set<nodeId> maximalCliqueIds;
	list<nodeIdSet*> ConnectedComponents;
		
	MsgNodeSet msgNodesSet;
	CliqueNodeSet cliqueNodesSet;
	list<TreeNode*> roots;
	FormulaMgr FM;
	//FormulaMgr* FMPtr;
	bool CDCL;
		
	void initCliqueNodes(unordered_map<nodeId,varSet*>& cliques);
//	void directTree(TreeNode* root,TreeNode* parent); //directs tree from the root
	void moveContainingBranches(TreeNode* root); //moves containing branches downstream
	void clearEdgeDirection();
	void clearEdgeDirection(const nodeIdSet& CC);
	
	string printRec(TreeNode* root);
	void rootInitialization(TreeNode* root, const nodeIdSet& CC, bool first=true); //initializes per root (or connected component)
	void globalInit(); //intialization actions that are carried out once for all CC
	
	static void removeComment(std::string &line);
	static bool onlyWhitespace(const std::string &line);
	void parseLine(const std::string &line, size_t const lineNo);
	void generateTermNodes(FormulaMgr& FM);
	
	
	int numOfDNFTerms;
	void constructTree();
	/*
	The code in this function is absed on ..
	*/
	void generateJunctionTree();
	//void triangulate();

	varType* PEO; //A perfect eliminationj ordering for the triangulated graph
	int numOfRelevantVars; //the numnber of vars that appear in at least one term
	
	varToContainingTermsMap varToContainingTerms;	
	varToNodeIdSet varToDNFTermIds;
	void mergeSets(const vector<nodeId>& nodeIds, varSet& retVal);
	void mergeSets(const nodeIdSet& nodeIds, varSet& retVal);
	void addEdgesToContainingNode(nodeId merged, nodeIdSet& orig);
	void addMsgNode(nodeId C1, nodeId C2);
	void setsContainingAllNodes(const varSet& vars, nodeIdSet& containVars);
	void deleteCliqueNode(CliqueNode* toDelete);
	void removeCliqueNode(nodeId cnId);
	nodeIdSet mergeContainedNodes(CliqueNode* cn);
	
	void getConnectedComponents();	
	size_t largestMsgWeight;
	void directPrim(TreeNode* root, const nodeIdSet& CC);
	size_t getEdgeWeight(nodeId src, nodeId dest);
	CliqueNode* getFirstCliqueNode(const nodeIdSet& CC);
	CliqueNode* getLargestDNFNode(const nodeIdSet& CC);
	void initVarToProbDefault(int numOfVars, probType defaultProb = 0.5f);
	bool isAcyclicTree;	
	probType deterministicVarsApriori;
	/**
	deal with a variable that is deterministicly set to 1
	*/
	void dealWithDeterministicVars(const varSet& deterministicVars);

	/**
	deal with a variable that is deterministicly set to 1
	*/
	void dealWithPosDeterministicVar(varType v, std::queue<varType>& varQueue);
	/**
	deal with a variable that is deterministicly set to 0
	*/
	void dealWithNegDeterministicVar(varType v);
	varSet deterministicVars;
	nodeIdSet getContainingNodeIfExists(CliqueNode* DNFTermNode);
	nodeIdSet getContainingNodeIfExists(const CTerm&);
	//bool hasDBJT() const;
	//varSet constraintOnlyVars;
	CliqueNode* getRandomNonDNFNode(const nodeIdSet& CC);
	CliqueNode* getSmallestNonDNFNode(const nodeIdSet& CC);	
	CliqueNode* getNonDNFNodeWithMaxTerms(const nodeIdSet& CC);
	CliqueNode* mergeNodes(const nodeIdSet& NIS);	

	void generateMsgNodes(const EdgesMap& edges);
	TreeGen* triangulateAndGetSize(Eliminator* elim,size_t& maxClique, size_t& maxSep);
	enum ElimType { Order, CliqueSize, NumFactors };

	Eliminator* executeEliminator(ElimType type, size_t& sepWidth, size_t& cliqueWidth, EdgesMap& EM);
	CliqueNode* getHighestDegreeSep(const nodeIdSet& CC);
public:
	size_t largestTreeNode;
	static int cnfFile_numOfVars;
	static int largestVar;
	long exportTimeMSec;
//	void exportFormulaToCNFFile(string filePath) const;
	int cnfFile_numOfTerms;
	void getCliquNodeIds(nodeIdSet& cliqueNodeIds, bool nonDNF);
	void getMsgNodeIds(nodeIdSet& msgNodeIds);
	ContractedJunctionTree(boost::unordered_map<nodeId,varSet*>& cliques,
		boost::unordered_map<nodeId, nodeIdSet>& treeEdges, nodeIdSet& DNFTerms, varToProbMap* varToProb);

	ContractedJunctionTree(string cnfFile, string elimOrderFile="", string elimOrder="");

//	ContractedJunctionTree(const junctionTreeState& JTS);

//	ContractedJunctionTree(FormulaMgr* FM);

	static ContractedJunctionTree* getInstance(){
		return instance;
	}

	static varSet& globalProbAssignedVars(){
		return global_probAssignedVars;
	}

	size_t getNumOfNodes(){
		return cliqueNodes.size()+msgNodes.size();
	}
	
	const varToNodeIdSet& varToDNFTerms(){
		return varToDNFTermIds;
	}

	FormulaMgr& getFormulaMgr(){
		return FM;
		//return *FMPtr;
	}
	const list<TreeNode*>& getRoots() const{
		return roots;
	}

	probType getEvidenceProb() const{
		return deterministicVarsApriori;
	}

	
	static nodeId getNextNodeId();
	
	
	const nodeIdSet& getNodeNbs(nodeId nId) const{
		if(contractedTreeEdges.find(nId) != contractedTreeEdges.end()){
			return contractedTreeEdges.at(nId);	
		}
		return ContractedJunctionTree::emptyNodeIdSet;
	}

	void addMsgNode(MsgNode* msg);
	void addCliqueNode(CliqueNode* cn);

	~ContractedJunctionTree();
	double getNaiveLargestTreeFactor() const;

	void initRandomly(); //initializes the tree by rooting it at an arbitrary node
	void initByDtreeRoot(const std::list<varType>& rootCutset); //initializes the tree by rooting it at a msg node containing rootCutset	
	MsgNode* getMsgNode(nodeId id) const;
	CliqueNode* getCliqueNode(nodeId id) const;
	TreeNode* getNode(nodeId id);
	string print() const;
	string printTree() const;
	
	CliqueNode* getLargestNonDNFNode(const nodeIdSet& CC);
	CliqueNode* gethHighestDegreeNode(const nodeIdSet& CC);
	static bool areProbsSame(probType d1, probType d2);
	bool isTreeAcyclic(){
		return isAcyclicTree;
	}
	bool hasDBJT();
	//DEBUG
	bool validateTermIds();
	bool validateTermDistInTree();
	void validateTermDistInTree(TreeNode* root,DBS& termDBS);	

	const CliqueNodeSet& getCliqueNodeSet() const{
		return cliqueNodesSet;
	}
};













#endif
