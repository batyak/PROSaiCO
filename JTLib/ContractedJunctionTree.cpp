#include "ContractedJunctionTree.h"
#include "Utils.h"
#include "CliqueNode.h"
#include "MsgNode.h"
#include "Convert.h"
#include "Assignment.h"
#include "CliqueSizeEliminator.h"
#include "NumOfFactorsEliminator.h"
#include "TreeGen.h"
#include <boost/algorithm/string.hpp>
#include "FactorTreeNode.h"
#include "BacktrackSM.h"
#include "OrderedEliminator.h"
#include <stdlib.h>     /* srand, rand */
#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost::algorithm;

using namespace boost;
#define COMMENT_CHAR "c "

ContractedJunctionTree* ContractedJunctionTree::instance=NULL;
varSet ContractedJunctionTree::global_probAssignedVars;
size_t ContractedJunctionTree::largestClique;
nodeId ContractedJunctionTree::runningIdNum;
nodeIdSet ContractedJunctionTree::emptyNodeIdSet;
int ContractedJunctionTree::cnfFile_numOfVars=0;
int ContractedJunctionTree::largestVar=0;

/*
ContractedJunctionTree::ContractedJunctionTree(const junctionTreeState& JTS){
	this->exportTimeMSec = 0;
	largestVar = 0;
	varType currVarIdx = 1;
	unordered_map<varType, varType> idxedVars;
	FM.init(1000,1000);
	varProbNode* currVarEntry = JTS.varsAndProbs;	
	while(currVarEntry != NULL){
		varType v = currVarEntry->var;
		if(idxedVars.find(v) == idxedVars.end()){ //we have not seen this var before
			varType mapped = currVarIdx++; //map it by running index
			idxedVars[v] = mapped;  //map to indexed var
			prob pr_v_1=currVarEntry->varProb;
			FM.addVar(mapped,pr_v_1,1.0 - pr_v_1);		
		}		
		currVarEntry = currVarEntry->next;
	}

	DNFTermNode* currTerm = JTS.DNFTerms;
	while(currTerm != NULL){
		varNode* currTermVar = currTerm->termHead;
		if(currTermVar != NULL){ //disregard empty DNF terms
			nodeId termId = getNextNodeId();
			varSet dnfTerm;		
			while(currTermVar != NULL){				
				varType v_mapped = idxedVars.at(currTermVar->var);
				dnfTerm.insert(v_mapped);
				varToContainingTerms[v_mapped].containingTerms.insert(termId);											
				currTermVar = currTermVar->next;
			} 
			//after all vars have been inserted into dnfTerm
			CliqueNode* cn = new CliqueNode(termId,dnfTerm, true);
			cliqueNodes[cn->getId()] = cn;
			cliqueNodesSet.insert(cn);		
			currTerm = currTerm->next;
		}
	}
	numOfDNFTerms = cliqueNodesSet.size();
	largestVar = currVarIdx;

	clock_t t = clock();
	// Current date/time based on current system
	time_t now = time(0);
	// Convert now to tm struct for local timezone
	tm* localtm = localtime(&now);
	stringstream filePath;
	char timestamp[16];
	strftime(timestamp,16,"%y%m%d_%H_%M_%S",localtm);
	filePath << "C:\\ModelCounting\\cachet-wmc-1-22\\queryLineage\\" << "query_" << timestamp << ".cnf";
	exportFormulaToCNFFile(filePath.str());
	exportTimeMSec = clock() - t;

	constructTree();

}*/
/*
void ContractedJunctionTree::exportFormulaToCNFFile(string filePath) const{
	const char* varFormat = "w %d %.6f \n";

	ofstream cnfFile;
	cnfFile.open(filePath.c_str(), std::ofstream::in);
	if(!cnfFile.is_open()){
		cout << " Cannot open file " << filePath << endl;
		return;
	}
	cnfFile << "c writing cnf formula from exportFormulaToCNFFile. \n";
	char header[60];
	sprintf(header, "p cnf %d %d \n", FM.getNumVars() , numOfDNFTerms);	
	cnfFile << header;
	const vector<CVariable>& vars = FM.getVars();
	for(vector<CVariable>::const_iterator cit = vars.begin(); cit != vars.end(); ++cit){
		const CVariable& var = *cit;
		if(var.getV() <= 0)
			continue;
		varType toWrite = var.getV();	
		char varStr[50];
		sprintf(varStr, varFormat, toWrite, (1.0-var.getPos_w())); //1.0- cit->second because this is a CNF file
		cnfFile << varStr;
	}
	cnfFile.flush();

	for(CliqueNodeSet::const_iterator cliqueIt = cliqueNodesSet.cbegin() ; cliqueIt != cliqueNodesSet.cend() ; ++cliqueIt){
		if(!(*cliqueIt)->isDNFTerm()) //we want to print only DNF terms
			continue;

		varSet cliqueMems = (*cliqueIt)->getVars();
		std::stringstream dnfTerm;
		for(varSet::const_iterator varIt = cliqueMems.cbegin() ; varIt != cliqueMems.cend() ; ++varIt){
			//varType dnfMappedVar = idxMap.at(*varIt);
			dnfTerm << *varIt << " ";
		}
		dnfTerm << "0" << endl;
		cnfFile << dnfTerm.str();
	}
	cnfFile.flush();
	cnfFile.close();

}
*/

ContractedJunctionTree::ContractedJunctionTree(string cnfFilePath, string elimOrderFile, string elimOrder){
	clock_t t = clock();
	deterministicVarsApriori = 1.0;
	runningIdNum = 1;		
	std::ifstream file;
	file.open(cnfFilePath.c_str());
	if (!file){
		cout << "CFG: File " + cnfFilePath + " couldn't be found!\n" << endl;
		return;
	}

	std::string line;
	size_t lineNo = 0;
	while (std::getline(file, line)){
		lineNo++;
		std::string temp = line;
		trim(temp);

		if (temp.empty()|| temp.compare("c")==0)
			continue;
		if(temp[0]=='c')
			continue;
		
		removeComment(temp);
		if (onlyWhitespace(temp))
			continue;

		parseLine(temp, lineNo);
	}
	file.close();	
	if(!Params::instance().weightsFile.empty()){
		FM.readLitWeightsFromLmapFile(Params::instance().weightsFile);
	}
	if(!elimOrderFile.empty()){
		FM.readOrderFile(elimOrderFile);
	}
	if(!elimOrder.empty()){
		FM.readOrderString(elimOrder);
	}
	long timeToReadFile = clock()-t;
	cout << " Time to read file: " << timeToReadFile/1000 << " seconds" << endl;
	bool isSAT;
	deterministicVarsApriori = FM.preprocessBCP(isSAT);		
	if(isSAT){
		deterministicVarsApriori = (Params::instance().ADD_LIT_WEIGHTS ? -1.0 : 0.0);
	}
	generateTermNodes(FM);	
	//to make sure that the term ids are correctly associated with their index in the vector
	FM.setTermIds();
	FM.generateWatchMap();
	FM.readPMAPFile(); //will do nothing if it does not exist

	constructTree();
	validateTermIds();
	cout << " Time to construct tree: " << (clock()-t)/1000 << " seconds" << endl;	

	ContractedJunctionTree::instance = this;
	
}
Eliminator* ContractedJunctionTree::executeEliminator(ElimType type, size_t& sepWidth, size_t& cliqueWidth, EdgesMap& EM){		
		EM.clear();
		Eliminator* elim;
		switch(type){
		case Order:{
				elim = new OrderEliminator(cliqueNodes,varToContainingTerms,FM.getElimOrder());
				break;
			};
		case CliqueSize:{
				elim = new CliqueSizeEliminator(cliqueNodes,varToContainingTerms);
				break;
			};
		case NumFactors:{
				elim = new NumOfFactorsEliminator(cliqueNodes,varToContainingTerms);
				break;
			}								 
		}
		TreeGen* TG = triangulateAndGetSize( elim, cliqueWidth , sepWidth);
		EM.insert(TG->getTreeEdges().begin(), TG->getTreeEdges().end());
		delete TG;
		return elim;

}


bool ContractedJunctionTree::validateTermIds(){
	const std::vector<CTerm>& terms = FM.getTerms();
	for(int i=0 ; i < terms.size() ; i++){
		nodeId termid = terms[i].getId();
		assert(termid == i);
		if(termid != i)
			return false;
	}
	return true;
}


bool ContractedJunctionTree::validateTermDistInTree(){
	const std::vector<CTerm>& terms=FM.getTerms();	
	DBS termsDBS;
	termsDBS.resize(terms.size(),false);
	for(std::list<TreeNode*>::iterator rootIt = roots.begin() ; rootIt != roots.end() ; ++rootIt){
		TreeNode* root = *rootIt;
		validateTermDistInTree(root,termsDBS);
	}	
	assert(termsDBS.count() == terms.size());
	return (termsDBS.count() == terms.size());
}


void ContractedJunctionTree::validateTermDistInTree(TreeNode* root,DBS& termDBS){	
	if(root->children.empty()){ //leaf node
		assert(root->isDNFTerm());
		const DBS& leafDNFIds = root->getDNFIds();
		assert(leafDNFIds.count() == 1);
		size_t dnfIdx = leafDNFIds.find_first();
		CTerm& t = FM.getTerm(dnfIdx);
		assert(t.getId() == dnfIdx);
		assert(!termDBS[dnfIdx]); //can belong to only a single leaf node
		termDBS[dnfIdx]=true;
		return;
	}
	for(std::list<TreeNode*>::const_iterator it = root->children.begin() ; it != root->children.end() ; ++it){
		validateTermDistInTree(*it,termDBS);
	}	
}

void ContractedJunctionTree::constructTree(){
	
	this->largestMsgWeight = 0;
	this->largestTreeNode = 0;
	
	Eliminator* elim = NULL;
	if(!FM.getElimOrder().empty()){
		elim = new OrderEliminator(cliqueNodes,varToContainingTerms,FM.getElimOrder());
	}
	else{
		elim = new CliqueSizeEliminator(cliqueNodes,varToContainingTerms);
		//elim = new NumOfFactorsEliminator(cliqueNodes,varToContainingTerms);
	}
	TreeGen TG(elim);
	TG.generateTree();
	const EdgesMap& edges =  TG.getTreeEdges();
	//elim created new nodes
	this->runningIdNum = elim->getNextNodeId();

	//DEBUG: make sure its a tree
	//bool test = TreeGen::validateTree(contractedTreeEdges);
	
	//construct msg nodes
	EdgesMap::const_iterator end = edges.end();
	for(EdgesMap::const_iterator edgeIt = edges.cbegin() ; edgeIt != end ; ++edgeIt){
		const nodeIdSet& nbrs = edgeIt->second;
		nodeIdSet::const_iterator nbrsEnd = nbrs.cend();
		for(nodeIdSet::const_iterator nodeIt = nbrs.cbegin() ; nodeIt != nbrsEnd ; ++nodeIt){
			addMsgNode(edgeIt->first,*nodeIt);
		}
	}	
	//DEBUG: make sure its a tree
	//TreeGen::validateTree(contractedTreeEdges);
	ContractedJunctionTree::instance = this;
	isAcyclicTree = elim->isAcyclicTree();
	//DEBUG - new
	cliqueNodesSet.clear();
	cliqueNodesSet.insert(elim->getMaxCliques().begin(), elim->getMaxCliques().end());
	
	delete elim;	
	cout << " Largest clique node is " << this->largestTreeNode << endl;	
	cout << " Largest Msg node is " << this->largestMsgWeight << endl;

}

TreeGen* ContractedJunctionTree::triangulateAndGetSize(Eliminator* elim,	size_t& maxClique, size_t& maxSep){
	maxClique = 0;
	maxSep = 0;
	TreeGen* TG = new TreeGen(elim);
	TG->generateTree();
	const EdgesMap& edges =  TG->getTreeEdges();
	const unordered_map<nodeId,CliqueNode*>& elimcliqueNodes = elim->getCliqueNodesMap();
	EdgesMap::const_iterator end = edges.end();
	for(EdgesMap::const_iterator edgeIt = edges.cbegin() ; edgeIt != end ; ++edgeIt){
		nodeId C1_id = edgeIt->first;
		CliqueNode* C1 = elimcliqueNodes.at(C1_id);
		maxClique = (C1->getVars().size() > maxClique) ? C1->getVars().size() : maxClique;

		const nodeIdSet& nbrs = edgeIt->second;
		nodeIdSet::const_iterator nbrsEnd = nbrs.cend();

		for(nodeIdSet::const_iterator nodeIt = nbrs.cbegin() ; nodeIt != nbrsEnd ; ++nodeIt){
			nodeId C2_id = *nodeIt;
			CliqueNode* C2 = elimcliqueNodes.at(C2_id);
			varSet C1_C2_Int;
			//calculate the intersection between connected nodes				
			Utils::intersect((C1->getVars()), (C2->getVars()),C1_C2_Int);
			maxSep = (C1_C2_Int.size() > maxSep) ? C1_C2_Int.size() : maxSep;
		}
	}	
	return TG;
}

void ContractedJunctionTree::generateMsgNodes(const EdgesMap& edges){
	msgNodes.clear();
	msgNodesSet.clear();
	contractedTreeEdges.clear();	

	EdgesMap::const_iterator end = edges.end();
	for(EdgesMap::const_iterator edgeIt = edges.cbegin() ; edgeIt != end ; ++edgeIt){
		const nodeIdSet& nbrs = edgeIt->second;
		nodeIdSet::const_iterator nbrsEnd = nbrs.cend();
		for(nodeIdSet::const_iterator nodeIt = nbrs.cbegin() ; nodeIt != nbrsEnd ; ++nodeIt){
			addMsgNode(edgeIt->first,*nodeIt);
		}
	}	
}

void ContractedJunctionTree::generateTermNodes(FormulaMgr& FM){
	ContractedJunctionTree::instance = this;
	vector<CTerm>& terms = FM.getTerms();
	//sorts in descending term size
	std::sort(terms.begin(), terms.end());
	vector<CTerm>::const_iterator end = terms.end();
	nodeId termIdx = 0;
	for(vector<CTerm>::const_iterator it = terms.begin() ; it != end ; ++it, termIdx++){
		const CTerm& term = *it;
		//find out whether we already encountered a DNF term over the same variable set
		CliqueNode* cn = NULL;				
		nodeIdSet containingNodeIds = getContainingNodeIfExists(term);				
		cn = (containingNodeIds.empty()) ? NULL : cliqueNodes.at(*containingNodeIds.cbegin());
		if(cn == NULL){
			nodeId cliqueNodeId = getNextNodeId();								
			varSet absDnfTerm;
			const vector<varType>& literals = term.getLits();
			vector<varType>::const_iterator lEnd = literals.end();
			for(vector<varType>::const_iterator it = literals.begin() ; it != lEnd ; ++it){
				varType v=*it;
				varType vAbs = (v > 0) ? v : 0-v;
				absDnfTerm.insert(vAbs);
				varToContainingTerms[vAbs].containingTerms.insert(cliqueNodeId);
			}
			cn = new CliqueNode(cliqueNodeId,absDnfTerm, false);
			cliqueNodes[cn->getId()] = cn;
			cliqueNodesSet.insert(cn);
		}
		cn->addDNFTerm(termIdx,true);
	}

}

void ContractedJunctionTree::parseLine(const std::string &line, size_t const lineNo)
{
	string buf; // Have a buffer string
    stringstream ss(line); // Insert the string into a stream
    vector<string> tokens; // Create vector to hold our words	
	boost::split(tokens,line,boost::is_any_of(" \t"),boost::token_compress_on);
	
	if(tokens.at(0).compare("p") == 0){
		cnfFile_numOfVars=Convert::string_to_T<int>(tokens[2]);		
		cout << " Number of vars: " << cnfFile_numOfVars << endl;
		cnfFile_numOfTerms = Convert::string_to_T<int>(tokens[3]);
		cout << " Number of terms: " << cnfFile_numOfTerms << endl;
		//initVarToProbDefault(cnfFile_numOfVars);
		largestVar = cnfFile_numOfVars;
		FM.init(cnfFile_numOfVars,cnfFile_numOfTerms);
		return;
	}
    
	if(tokens.at(0).compare("w") == 0){
		varType v = Convert::string_to_T<varType>(tokens[1]);
		largestVar = (v > largestVar) ? v : largestVar;
		probType v_weight=Convert::string_to_T<probType>(tokens[2]);
		probType v_pos,v_neg;
		if(tokens.size() >3){
			probType v_neg_weight = Convert::string_to_T<probType>(tokens[3]);
			FormulaMgr::getProbs(v_weight,v_neg_weight,v_pos,v_neg);
		}
		else{
			FormulaMgr::getProbs(v_weight,v_pos,v_neg);
		}
	//	probType v_pos = (v_weight < 0) ? 1.0 : 1.0-v_weight;  //we are looking at DNF 
	//	probType v_neg = (v_weight < 0) ? 1.0 : v_weight;
		FM.updateVar(v,v_pos,v_neg);				
		cnfFile_numOfVars--;
	}
	else{ //must be a CNF term
		varSet dnfTerm;
		for (vector<string>::iterator token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter){
			if((*token_iter).empty() || onlyWhitespace(*token_iter)) continue;

			varType v = Convert::string_to_T<varType>(*token_iter);	
			if(v != 0){
				dnfTerm.insert(v);
			}
		}			
		if(!dnfTerm.empty()){
				//the dnf term node containing the literals				
				FM.addTerm(dnfTerm);
			}
		}			
}

//check if DNFTerm node is contained in a previously encountered node
nodeIdSet ContractedJunctionTree::getContainingNodeIfExists(const CTerm& term){
	bool first = true;
	nodeIdSet containingNodeIds;	
	varSet termVars;
	term.getDNFVarSet(termVars);
	varSet::const_iterator end = termVars.cend();
	for(varSet::const_iterator varit = termVars.cbegin() ; varit != end ; ++varit){
		varType v = *varit;
		if(first){
			//initialized with all nodes containing v
			containingNodeIds.insert(varToContainingTerms[v].containingTerms.cbegin() , varToContainingTerms[v].containingTerms.cend());
			first = false;
		}
		else{
			Utils::intersect(varToContainingTerms[v].containingTerms, containingNodeIds);
			if(containingNodeIds.empty()){
				return containingNodeIds;
			}
		}
	}
	return containingNodeIds;
}

//check if DNFTerm node is contained in a previously encountered node
nodeIdSet ContractedJunctionTree::getContainingNodeIfExists(CliqueNode* DNFTermNode){
	bool first = true;
	nodeIdSet containingNodeIds;
	const varSet& termVars = DNFTermNode->getVars();

	varSet::const_iterator end = termVars.cend();
	for(varSet::const_iterator varit = termVars.cbegin() ; varit != end ; ++varit){
		varType v = *varit;
		v = (v > 0) ? v : 0-v;
		if(first){
			//initialized with all nodes containing v
			containingNodeIds.insert(varToContainingTerms[v].containingTerms.cbegin() , varToContainingTerms[v].containingTerms.cend());
			first = false;
		}
		else{
			Utils::intersect(varToContainingTerms[v].containingTerms, containingNodeIds);
			if(containingNodeIds.empty()){
				return containingNodeIds;
			}
		}
	}
	return containingNodeIds;
}


nodeId ContractedJunctionTree::getNextNodeId(){
	return runningIdNum++;
}

void ContractedJunctionTree::removeComment(std::string &line)
{
	if (line.find(COMMENT_CHAR) != line.npos)
		line.erase(line.find(COMMENT_CHAR));
}

bool ContractedJunctionTree::onlyWhitespace(const std::string &line)
{
	return (line.find_first_not_of(' ') == line.npos);
}

void ContractedJunctionTree::addMsgNode(nodeId C1_id, nodeId C2_id){
	CliqueNode* C1 = cliqueNodes.at(C1_id);
	CliqueNode* C2 = cliqueNodes.at(C2_id);
	largestTreeNode = (C1->getVars().size() > largestTreeNode) ? C1->getVars().size() : largestTreeNode;
	largestTreeNode = (C2->getVars().size() > largestTreeNode) ? C2->getVars().size() : largestTreeNode;
	//DEBUG
	//string c1str = C1->shortPrint();
	//string c2str = C2->shortPrint();

	varSet C1_C2_Int;
	//calculate the intersection between connected nodes				
	Utils::intersect((C1->getVars()), (C2->getVars()),C1_C2_Int);
	MsgNode* C1_C2_Msg;
	MsgNode temp(0, C1_C2_Int,false);
	MsgNodeSet::iterator tnsIT = msgNodesSet.find(&temp);
	if(tnsIT == msgNodesSet.end()){
		C1_C2_Msg =  new MsgNode(getNextNodeId(),C1_C2_Int,false);					
		msgNodes[C1_C2_Msg->getId()] = C1_C2_Msg;
		msgNodesSet.insert(C1_C2_Msg);
		largestMsgWeight = (largestMsgWeight < C1_C2_Msg->getVars().size()) ? C1_C2_Msg->getVars().size() : largestMsgWeight;
	}
	else{
		C1_C2_Msg = *tnsIT;
	}	
	//DEBUG
	//string msgPrint = C1_C2_Msg->shortPrint();
	nodeId msgId = C1_C2_Msg->getId();
	contractedTreeEdges[C1_id].insert(msgId);
	contractedTreeEdges[C2_id].insert(msgId);
	contractedTreeEdges[msgId].insert(C1_id);
	contractedTreeEdges[msgId].insert(C2_id);
}

//returns a set of clique ids which contain all members in vars, or an empty set if none exist.
void ContractedJunctionTree::setsContainingAllNodes(const varSet& vars, nodeIdSet& containVars){	
	bool firstIt = true;
	varSet::const_iterator end = vars.cend();
	for(varSet::const_iterator varIt = vars.cbegin() ; varIt != end; ++varIt){
		varInfo& vInf = varToContainingTerms[*varIt];
		const nodeIdSet& vTerms = vInf.containingTerms;
		if(firstIt){
			containVars.insert(vTerms.cbegin(), vTerms.cend());			
			firstIt = false;
		}
		else{			
			Utils::intersect(vTerms,containVars);
			if(containVars.size() <= 1) //we know that there must exist at least a single term containing all vars
				break;
		}

	}
}

void ContractedJunctionTree::deleteCliqueNode(CliqueNode* toDelete){	
	cliqueNodes.erase(toDelete->getId());
	delete toDelete;
}

void ContractedJunctionTree::initCliqueNodes(unordered_map<nodeId,varSet*>& cliques){
	largestClique = 0;	
	for(unordered_map<nodeId,varSet*>::iterator cliqueIt = cliques.begin() ; cliqueIt != cliques.end() ; ++cliqueIt){	
		bool isDNF = (DNFTerms.find(cliqueIt->first) != DNFTerms.end());
		CliqueNode* currNode = new CliqueNode(cliqueIt->first,*cliqueIt->second,isDNF);
		cliqueNodes[currNode->getId()] = currNode;		
		largestClique = (largestClique < currNode->getVars().size()) ? currNode->getVars().size(): largestClique;
	}
}

TreeNode* ContractedJunctionTree::getNode(nodeId id){
	if(cliqueNodes.find(id)  != cliqueNodes.end()){
		return cliqueNodes.at(id);
	}
	else{
		return msgNodes.at(id);
	}
}

MsgNode* ContractedJunctionTree::getMsgNode(nodeId id) const{
	return msgNodes.at(id);
}
CliqueNode* ContractedJunctionTree::getCliqueNode(nodeId id) const{
	return cliqueNodes.at(id);
}
//TODO:
//Prepare tree containing only positive variables
void ContractedJunctionTree::directPrim(TreeNode* root, const nodeIdSet& CC){
	if(CC.size() <= 1){
		return; //tree is either empty or contains a single node
	}
	//first get the node ids of all clique and msg nodes.
	nodeIdSet allNodsIds;
	//remove DNF term nodes - they will be added later one
	nodeIdSet::const_iterator CCend = CC.cend();
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != CCend ; ++nit){
		TreeNode* currNode = getNode(*nit);
		if(!currNode->isDNFTerm()){
			allNodsIds.insert(*nit);
		}
	}

	unordered_map<nodeId, size_t> key;
	size_t arrSize = (largestMsgWeight > 0) ? largestMsgWeight+1: 2;
	nodeIdSet* weightArr = new nodeIdSet[arrSize]; //weightArr[i] contains the set of node ids whose weight mark is i

	nodeIdSet::const_iterator end = allNodsIds.cend();
	for(nodeIdSet::const_iterator nodeIt = allNodsIds.cbegin() ; nodeIt != end ; ++nodeIt ){		
		key[*nodeIt] = 0;
	}		
	
	allNodsIds.erase(root->getId());
	weightArr[0]=allNodsIds;
	weightArr[1].insert(root->getId());
	key[root->getId()]=1;
	size_t maxIdx = 1;
	root->setParent(NULL);

	while(!allNodsIds.empty()){ //still have nodes that are not in the tree
		nodeId u = *(weightArr[maxIdx].cbegin()); //get mx node
		weightArr[maxIdx].erase(u);
		allNodsIds.erase(u);
		TreeNode* uNode = getNode(u);		
		const nodeIdSet& uNbrs = contractedTreeEdges.at(u);
		for(nodeIdSet::const_iterator nbtIt = uNbrs.cbegin() ; nbtIt != uNbrs.cend() ; ++nbtIt){ //iterate over node neighbors
			nodeId v= *nbtIt;
			if(allNodsIds.find(v) != allNodsIds.end()){ //nbr has not yet been added to the tree
				size_t edgeWeight = getEdgeWeight(u,v);
				if(edgeWeight > key.at(v)){ //update v's key
					weightArr[key[v]].erase(v);
					weightArr[edgeWeight].insert(v);
					key[v]=edgeWeight;
					maxIdx = (maxIdx < edgeWeight) ? edgeWeight : maxIdx;
					TreeNode* vNode = getNode(v);
					TreeNode* vParent = vNode->getParent();
					//need to remove the child from the parent
					if(vParent != NULL){
						vParent->removeChild(vNode);
					}		
					vNode->setParent(uNode);
					uNode->addChild(vNode);
				}
			}
		}		
		while(weightArr[maxIdx].empty() && maxIdx>0) 
			maxIdx--;
	}

}

size_t ContractedJunctionTree::getEdgeWeight(nodeId src, nodeId dest){
	if(msgNodes.find(src) != msgNodes.end())
		return msgNodes.at(src)->getVars().size();
	else{ //dest must be a message node
		return msgNodes.at(dest)->getVars().size();
	}
}


/**
This method will also construct the almond tree. While iterating over the contracted tree it will do the following:
1. If root is a clique node:
	1.1 If root has two children S1, S2 such that S1 \supset S2, then we detach S2 from the root and place it as a child of S2.
	1.2 Otherwise, we check if one of the children is subsumed in the parent of r. In that case, we connect S1 to its parent.
2. If r is a msg node then:
	2.1 If root has two msg node children S1, S2 such that S1 \supset S2, then we detach S2 from the root and place it as a child of S2.
**/
void ContractedJunctionTree::moveContainingBranches(TreeNode* root){
	list<TreeNode*>& r_children = root->children;
	for(list<TreeNode*>::iterator childIt = r_children.begin() ; childIt != r_children.end() ; ){
		TreeNode* child = *childIt;
		TreeNode* containingSib = child->getContainingSibling(r_children);
		if(containingSib != NULL){
			childIt = r_children.erase(childIt);
			TreeNode* newParent = containingSib->children.front();
			newParent->children.push_back(child);
			child->parent = newParent;
		}
		else{
			++childIt;
		}
	}
	for(list<TreeNode*>::const_iterator childIt = r_children.begin() ; childIt != r_children.end() ; ++childIt){
		moveContainingBranches(*childIt);
	}
}

void ContractedJunctionTree::clearEdgeDirection(){
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueNodesIt = cliqueNodes.cbegin() ; cliqueNodesIt != cliqueNodes.cend() ; ++cliqueNodesIt ){
		cliqueNodesIt->second->children.clear();
		cliqueNodesIt->second->setParent(NULL);
	}
	
	for(unordered_map<nodeId,MsgNode*>::const_iterator msgNodesIt = msgNodes.cbegin() ; msgNodesIt != msgNodes.cend() ; ++msgNodesIt ){
		msgNodesIt->second->children.clear();
		msgNodesIt->second->setParent(NULL);
	}
}

void ContractedJunctionTree::getCliquNodeIds(nodeIdSet& cliqueNodeIds, bool nonDNF){
	unordered_map<nodeId,CliqueNode*>::const_iterator end = cliqueNodes.cend();
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueIt = cliqueNodes.cbegin() ; cliqueIt != end ; ++cliqueIt){		
		if(!nonDNF || !cliqueIt->second->isDNFTerm()){  
			cliqueNodeIds.insert(cliqueIt->first);
		}
	}
}

void ContractedJunctionTree::getMsgNodeIds(nodeIdSet& msgNodeIds){
	unordered_map<nodeId,MsgNode*>::const_iterator end = msgNodes.cend();
	for(unordered_map<nodeId,MsgNode*>::const_iterator msgIt = msgNodes.cbegin() ; msgIt != end ; ++msgIt){		
		msgNodeIds.insert(msgIt->first);
	}
}

//initializes the tree by rooting it at an arbitrary node
void ContractedJunctionTree::initRandomly(){		
	globalInit();	
	getConnectedComponents();
	for(list<nodeIdSet*>::const_iterator lIt = ConnectedComponents.begin() ; lIt != ConnectedComponents.end() ; ++lIt){ //run over each node in the connected components
		nodeIdSet* CCp = *lIt;
		CliqueNode* CCRoot = this->getRandomNonDNFNode(*CCp);
		//CliqueNode* CCRoot = this->gethHighestDegreeNode(*CCp);
		//CliqueNode* CCRoot = getHighestDegreeSep(*CCp);
		
		roots.push_back(CCRoot);
		rootInitialization(CCRoot, *CCp);		
	}
}

//initializes the tree by rooting it at a msg node containing rootCutset
void ContractedJunctionTree::initByDtreeRoot(const std::list<varType>& rootCutset){
	globalInit();	
	getConnectedComponents();
	if(ConnectedComponents.size() > 1){
		for(list<nodeIdSet*>::const_iterator lIt = ConnectedComponents.begin() ; lIt != ConnectedComponents.end() ; ++lIt){ //run over each node in the connected components
			nodeIdSet* CCp = *lIt;
			CliqueNode* CCRoot = this->getRandomNonDNFNode(*CCp);
			roots.push_back(CCRoot);
			rootInitialization(CCRoot, *CCp);		
		}
		cout << " Formula has " << ConnectedComponents.size() << " connected components, roots were chosen randomly " << endl;
		return;
	}
	nodeIdSet* CCp = *ConnectedComponents.begin();
	varSet nodeVars;
	for(std::list<varType>::const_iterator it = rootCutset.begin() ; it != rootCutset.end() ; ++it){
		nodeVars.insert(*it);
	}	

	TreeNode* root = NULL;
	size_t rootSize = 0;
	CliqueNodeSet::const_iterator end = cliqueNodesSet.end();
	//find the smallest clique node that contains thsi set of vars
	for(CliqueNodeSet::const_iterator it = cliqueNodesSet.begin() ; it != end ; ++it){
		CliqueNode* temp = *it;
		if(Utils::contains(temp->getVars(),nodeVars)){
			if(root == NULL || rootSize > temp->getVars().size()){
				root = temp;
				rootSize =  temp->getVars().size();
			}
		}
	}
	if(root == NULL){
		cout << " Did not find a node that contains " << Utils::printList(rootCutset) << endl;
		root = getRandomNonDNFNode(*CCp);
	}
	roots.push_back(root);
	rootInitialization(root,*CCp);	
}

CliqueNode* ContractedJunctionTree::getRandomNonDNFNode(const nodeIdSet& CC){
	/* initialize random seed: */	
	if(Params::instance().seed == 0){
		Params::instance().seed = (unsigned int)time(NULL);
	}
	srand (Params::instance().seed );

	/* generate secret number between 1 and 10: */
	int node = rand() % CC.size();

	nodeIdSet::const_iterator end = CC.cend();	
	nodeId i=0;
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit, i++){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			CliqueNode* temp = cliqueNodes.at(nId);
			if(!temp->isDNFTerm() && i >= node){
				return temp;
			}
		}
	}
	return NULL;
}

CliqueNode* ContractedJunctionTree::getHighestDegreeSep(const nodeIdSet& CC){
	MsgNode* largest = NULL;
	for(unordered_map<nodeId,MsgNode*>::const_iterator it = msgNodes.begin() ; 
			it != msgNodes.end() ; ++it){
		MsgNode* curr = it->second;
		if(largest == NULL || curr->getVarsBS().count() > largest->getVarsBS().count()){
			largest = curr;
		}
	}
	
	const nodeIdSet& nbrs = contractedTreeEdges.at(largest->getId());
	//now select the largest neighbor as the root	
	CliqueNode* root = NULL;
	for(nodeIdSet::const_iterator it = nbrs.begin() ; it != nbrs.end() ; ++it){
		CliqueNode* curr = cliqueNodes.at(*it);
		if(root == NULL || curr->getVarsBS().count() > root->getVarsBS().count()){
			root = curr;
		}
	}
	return root;
}


CliqueNode* ContractedJunctionTree::gethHighestDegreeNode(const nodeIdSet& CC){
	size_t maxChildren = 0;
	size_t maxCard = 0;
	CliqueNode* root = NULL;
	nodeIdSet::const_iterator end = CC.cend();	
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) == cliqueNodes.cend())
			continue;		
		CliqueNode* temp = cliqueNodes.at(nId);
		if(temp->isDNFTerm())
			continue;
		
		size_t nChildren = contractedTreeEdges.at(nId).size(); //num of distinct msg nebrs
		size_t nCard = 0; //max cardinality of nbrs combined
		if(nChildren >= maxChildren){
			maxChildren = nChildren;
			varSet sepVars; //calculate total sep-set
			//calculate children cardinality
			const nodeIdSet& nbrs = contractedTreeEdges.at(nId);
			for(nodeIdSet::const_iterator nbrit = nbrs.cbegin() ; nbrit != nbrs.end(); ++nbrit){
				MsgNode* nbr = msgNodes.at(*nbrit);
				sepVars.insert(nbr->getVars().begin(), nbr->getVars().end());
			}
			nCard = sepVars.size();
			if(nChildren > maxChildren || nCard > maxCard){
				maxChildren = nChildren;
				maxCard = nCard;
				root = temp;
			}				
		}
		
	}
	return root;
	
}

CliqueNode* ContractedJunctionTree::getSmallestNonDNFNode(const nodeIdSet& CC){
	nodeIdSet::const_iterator end = CC.cend();
	CliqueNode* retVal = NULL;
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			CliqueNode* temp = cliqueNodes.at(nId);
			if(!temp->isDNFTerm()){
				retVal = (retVal == NULL || retVal->getVars().size() > temp->getVars().size()) ? temp : retVal;
			}
		}
	}
	return retVal;
}

CliqueNode* ContractedJunctionTree::getLargestNonDNFNode(const nodeIdSet& CC){
	nodeIdSet::const_iterator end = CC.cend();
	CliqueNode* retVal = NULL;
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			CliqueNode* temp = cliqueNodes.at(nId);
			if(!temp->isDNFTerm()){
				retVal = (retVal == NULL || retVal->getVars().size() < temp->getVars().size()) ? temp : retVal;
			}
		}
	}
	return retVal;
}

CliqueNode* ContractedJunctionTree::getNonDNFNodeWithMaxTerms(const nodeIdSet& CC){
	nodeIdSet::const_iterator end = CC.cend();
	CliqueNode* retVal = NULL;
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			CliqueNode* temp = cliqueNodes.at(nId);
			if(!temp->isDNFTerm()){
				SubFormulaIF* tempForm = (SubFormulaIF*)temp;
				retVal = (retVal == NULL || ((SubFormulaIF*)retVal)->getDNFIds().size() < tempForm->getDNFIds().size()) ?
					 temp : retVal;
			}
		}
	}
	return retVal;

}

CliqueNode* ContractedJunctionTree::getLargestDNFNode(const nodeIdSet& CC){
	nodeIdSet::const_iterator end = CC.cend();
	CliqueNode* retVal = NULL;
	size_t maxSize =0;
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			CliqueNode* temp = cliqueNodes.at(nId);
			if(temp->isDNFTerm()){
				size_t nodeSize = temp->getVars().size();
				if(nodeSize > maxSize){
					retVal = temp;
					maxSize = nodeSize;
				}
			}
		}
	}
	return retVal;
}

CliqueNode* ContractedJunctionTree::getFirstCliqueNode(const nodeIdSet& CC){
	nodeIdSet::const_iterator end = CC.cend();
	for(nodeIdSet::const_iterator nit = CC.cbegin() ; nit != end; ++nit){
		nodeId nId = *nit;
		if(cliqueNodes.find(nId) != cliqueNodes.cend()){
			return cliqueNodes.at(nId);
		}
	}
	return NULL;
}
//returns the set of connected components in the resulting graph
void ContractedJunctionTree::getConnectedComponents(){	
	nodeIdSet cliqueNodes;
	nodeIdSet msgNodes;
	getCliquNodeIds(cliqueNodes,true); //all of the clique nodes in the graph (non-DNF nodes)
	getMsgNodeIds(msgNodes);
	while(!cliqueNodes.empty() || !msgNodes.empty()){ //still have nodes that do not belong to any tree
		CliqueNode* root = getFirstCliqueNode(cliqueNodes);		
		nodeIdSet* rootReachable = new nodeIdSet();		
		nodeIdSet rootVisited;
		root->getReachableNodes(*rootReachable,rootVisited);		
		this->ConnectedComponents.push_back(rootReachable);		
		Utils::subt(cliqueNodes,*rootReachable);		
		Utils::subt(msgNodes,*rootReachable);		
	}
}


//initializes per root (or connected component)
void ContractedJunctionTree::rootInitialization(TreeNode* root, const nodeIdSet& CC, bool first){	
	root->clearEdgeDirection();		
	directPrim(root, CC);
	moveContainingBranches(root);
	root->generateDNFLeaves();			
	root->generateSubFormulas();
}

//intialization actions that are carried out once for all CC
void ContractedJunctionTree::globalInit(){
	roots.clear();
	global_probAssignedVars.clear();
	SubFormulaIF::currNodelevel=0;
	FactorTreeInternal::currAssignmentlevel=0;
	//clearFactorCache();
}

double ContractedJunctionTree::getNaiveLargestTreeFactor() const{
	size_t retVal = 0;
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueIt =cliqueNodes.cbegin() ; cliqueIt != cliqueNodes.cend() ; ++cliqueIt){
		CliqueNode* currNode = cliqueIt->second;
		retVal = (currNode->getVars().size() > retVal) ? currNode->getVars().size() : retVal;
	}
	return pow(2.0,(double)retVal);
}

void ContractedJunctionTree::addMsgNode(MsgNode* msg){
	msgNodes[msg->getId()] = msg;		
	msgNodesSet.insert(msg);
}

void ContractedJunctionTree::addCliqueNode(CliqueNode* cn){
	cliqueNodes[cn->getId()] = cn;
	cliqueNodesSet.insert(cn);
}

string ContractedJunctionTree::printTree() const{
	std::stringstream ss;
	ss << " Tree has " << roots.size() << " roots" << endl;
	for(list<TreeNode*>::const_iterator rootIt = roots.begin() ; rootIt != roots.begin() ; ++rootIt){
		ss << (*rootIt)->printSubtree() << endl;
		
	}
	return ss.str();
	
}



ContractedJunctionTree::~ContractedJunctionTree(){
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueNodesIt = cliqueNodes.cbegin() ; cliqueNodesIt != cliqueNodes.cend() ; ){
		CliqueNode* cn = cliqueNodesIt->second;
		++cliqueNodesIt;
		delete cn;
	}
	
	for(unordered_map<nodeId,MsgNode*>::const_iterator msgNodesIt = msgNodes.cbegin() ; msgNodesIt != msgNodes.cend() ; ){
		MsgNode* mn = msgNodesIt->second;
		++msgNodesIt;
		delete mn;
	}	
	for(list<nodeIdSet*>::const_iterator lit = ConnectedComponents.begin() ; lit != ConnectedComponents.end() ; ){
		nodeIdSet* nis = *lit;
		++lit;
		delete nis;
	}
	global_probAssignedVars.clear();	
	instance = NULL;
}

/*
bool ContractedJunctionTree::runEveryNodeTestCase(string cnfFilePath,probType expectedProb){
	bool retVal = true;	
	std::cout << "Parsing cnf file: " << cnfFilePath << endl;
	//FormulaMgr* FMPtr = new FormulaMgr(cnfFilePath);
	ContractedJunctionTree* cjt = new ContractedJunctionTree(cnfFilePath);
	
	if(!cjt->runInferenceFromEveryNode(expectedProb)){
		retVal = false;
	}
	delete cjt;	
	return retVal;

}
*/
bool ContractedJunctionTree::areProbsSame(probType d1, probType d2){
	return std::fabs(d1 - d2) < 1E-6;
}

/*
Expectes the tree to contain a single root --> i.e to be connected
*/
/*
bool ContractedJunctionTree::runInferenceFromEveryNode(probType expectedProb){
	nodeIdSet allNodeIds;
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueIt = cliqueNodes.cbegin() ; cliqueIt != cliqueNodes.cend() ; ++cliqueIt){
		allNodeIds.insert(cliqueIt->first);
	}
	for(unordered_map<nodeId,MsgNode*>::const_iterator msgIt = msgNodes.cbegin() ; msgIt != msgNodes.cend() ; ++msgIt){
		allNodeIds.insert(msgIt->first);
	}

	bool retVal = true;	
	bool first = true;
	for(unordered_map<nodeId,CliqueNode*>::const_iterator cliqueIt = cliqueNodes.cbegin() ; cliqueIt != cliqueNodes.cend() ; ++cliqueIt){	
		CliqueNode* currRoot = cliqueIt->second;
		if(currRoot->isDNFTerm()){
			continue;
		}
		globalInit();		
		rootInitialization(currRoot, allNodeIds,first);
		roots.push_back(currRoot);
		cout << " The tree: " << currRoot->printSubtree() << endl;
		probType currProb = performInference();
		if(!areProbsSame(currProb,expectedProb)){
			cout << " BUG: The probability calculated from root: " << currRoot->shortPrint() << " is: " << currProb << endl;
			cout << " BUT The probability expected probability is: " << expectedProb << endl;							
			retVal = false;
		}
		
		cout << "largest factor when calculating from root: " << cliqueIt->second->shortPrint() << " is: " << TreeNode::maxNumOfEntries <<
			" using naive jt: " << pow(2.0,this->largestTreeNode) << endl; 
		currRoot->clearEdgeDirection(); //for next root
		first = false;
	}
	
	return retVal;
}
*/
/*
void ContractedJunctionTree::test(){

	
	Params::instance().CDCL = true;

	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\Grid\\Cnf\\Ratio_50\\3B3.cnf",0.560526)){
		cout << " 3B3: passed" << endl;
	}
	else{
		cout << " 3B3: failed" << endl;
	}

	string form_6="1.2.3+-1.4.5+3.5+-4.-5.1+2.-3";
	string varProbsStr_6="1:0.1,2:0.2,3:0.3,4:0.4,5:0.5";
	//DNFFormula formula6("C:\\ModelCounting\\cachet-wmc-1-22\\simpleNeg.cnf");
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\simpleNeg.cnf",0.5822)){
		cout << " TC6: passed" << endl;
	}
	else{
		cout << " TC6: failed" << endl;
	}

	string form_3="1.2.3.4+1.2.6+3.4.5+1.7+2.4.6";
	string varProbsStr_3="1:0.1,2:0.2,3:0.3,4:0.4,5:0.5,6:0.6,7:0.7";
	//DNFFormula formula3("C:\\ModelCounting\\cachet-wmc-1-22\\CNF_7_5.txt");
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\CNF_7_5.txt",0.833952)){
		cout << " TC3: passed" << endl;
	}
	else{
		cout << " TC3: failed" << endl;
	};

	


	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\testCases\\TC7.txt",0.00804874)){
		cout << " TC7: passed" << endl;
	}
	else{
		cout << " TC7: failed" << endl;
	}


	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\testCases\\CL.cnf",0.28125)){
		cout << " First CL test: passed" << endl;
	}
	else{
		cout << " First CL test: failed" << endl;
	}

	

	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\Grid\\Cnf\\Ratio_50\\2B3.cnf",0.390401)){
		cout << " 2B3: passed" << endl;
	}
	else{
		cout << " 2B3: failed" << endl;
	}

	



	

	

	


	

	


	
	
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\testCases\\TC7.5",0.23)){
		cout << " TC7.5: passed" << endl;
	}
	else{
		cout << " TC7.5: failed" << endl;
	}

	
	

	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\testCases\\inclusionTC2.cnf",0.0078125)){
		cout << " inclusionTC: passed" << endl;
	}
	else{
		cout << " inclusionTC: failed" << endl;
	}


	
	
	string form_4="1.2.3.4.5.6+1.2.6.7.8+1.3.5.9+5.7+1.3.9.10";
	string varProbsStr_4="1:0.1,2:0.2,3:0.3,4:0.4,5:0.5,6:0.6,7:0.7,8:0.8,9:0.9,10:0.1";
	//DNFFormula formula4("C:\\ModelCounting\\cachet-wmc-1-22\\cnf_10_5.txt");
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\cnf_10_5.txt",0.64130912)){
		cout << " TC4: passed" << endl;
	}
	else{
		cout << " TC4: failed" << endl;
	}

	
	string form_1="1.2.3.5.4+1.2.5.6.7+2.3.5.8.9";
	string varProbsStr_1="1:0.1,2:0.2,3:0.3,4:0.4,5:0.5,6:0.6,7:0.7,8:0.8,9:0.9";
	//DNFFormula formula1("C:\\ModelCounting\\cachet-wmc-1-22\\CNF_9_3.txt");
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\CNF_9_3.txt",0.97491232)){
		cout << " TC1: passed" << endl;
	}
	else{
		cout << " TC1: failed" << endl;
	}

	string form_2="2.4.15+1.2.3.4+1.3.5.12+1.2.4.6+2.3.4.7+1.4.8+2.3.9+4.10+2.11+1.12.13+5.12.14";
	string varProbsStr_2="1:0.1,2:0.2,3:0.3,4:0.4,5:0.5,6:0.6,7:0.7,8:0.8,9:0.9,10:0.1,11:0.11,12:0.12,13:0.13,14:0.14,15:0.15";
	//DNFFormula formula2("C:\\ModelCounting\\cachet-wmc-1-22\\CNF15_12.txt");
	if(runEveryNodeTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\CNF15_12.txt",0.849142)){
		cout << " TC2: passed" << endl;
	}
	else{
		cout << " TC2: failed" << endl;
	}

	
	

	
	

	//"1.2.3+1.2.6+1.2.4+4.6.7";
	//1:0.1,2:0.2,3:0.3,4:0.4,5:0.5,6:0.6";	
	if(runSingleRootTestCase("C:\\ModelCounting\\cachet-wmc-1-22\\testCases\\TC8.txt",false, 0.0397898)){
		cout << " TC8: passed" << endl;
	}
	else{
		cout << " TC8: failed" << endl;
	}
		
	


	
	


	

	
	

	




	

}
*/