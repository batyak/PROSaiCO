#include "DirectedGraph.h"
#include <boost/assert.hpp>
#include "Utils.h"

void DirectedGraph::initGraphStructures(){
	idxToOut = NULL;
	idxToIn = NULL;
	innerSize = vertices.size();
	if(innerSize == 0){
		return;
	}
	outEdges.resize(innerSize*innerSize);
	inEdges.resize(innerSize*innerSize);		
	indexToVar.reserve(innerSize+1);	
	//now map each var to an index (0-based)
	varSet::const_iterator end = vertices.cend();
	int index = 0;
	for(varSet::const_iterator it = vertices.cbegin() ; it != vertices.cend() ; ++it, ++index){
		varType v = *it;
		indexToVar.push_back(v);				
		varToIndex[v] = index;
	}
	idxToOut = new DBS*[innerSize+1];
	idxToIn = new DBS*[innerSize+1];	
	for(int i=0 ; i < (innerSize+1) ; i++){
		DBS* iOut;
		DBS* iIn;
		iOut = new DBS();
		iIn = new DBS();
		iOut->resize(innerSize,false);
		iIn->resize(innerSize,false);
		idxToOut[i] = iOut;
		idxToIn[i] = iIn;		
	}
	//idxToOut.clear();
	//idxToIn.clear();
}

varType DirectedGraph::getVarByIdx(size_t idx) const{
	return indexToVar[idx];
}
DirectedGraph* DirectedGraph::getSubgraph(const varSet& sub_vertices){
	DirectedGraph* subGraph = new DirectedGraph(sub_vertices);
	varSet::const_iterator varsEnd = sub_vertices.end();
	for(varSet::const_iterator varsIt = sub_vertices.begin() ; varsIt != varsEnd ; ++varsIt){
		varType from = *varsIt;
		size_t fromInd = varToIndex.at(from); //perform only once
		size_t fromIndSubGraph = subGraph->varToIndex.at(from); //perform only once
		for(varSet::const_iterator varsIt2 = sub_vertices.begin() ; varsIt2 != varsEnd ; ++varsIt2){
			varType to = *varsIt2;
			if(from == to){
				continue;
			}
			size_t toInd = varToIndex.at(to);		
			if(isEdge(fromInd,toInd)){ //i.e., if(isEdge(from,to))
				size_t toIndSubGraph = subGraph->varToIndex.at(to); //perform only once
				subGraph->addEdge(fromIndSubGraph,toIndSubGraph);
			}
			/*
			if(this->isEdge(from,to)){
				subGraph->addEdge(from,to);
			}*/
		}
	}		
	return subGraph;
}

void DirectedGraph::addEdge(varType src, varType dest){	
	size_t srcInd = varToIndex.at(src);
	size_t destInd = varToIndex.at(dest);
	addEdge(srcInd,destInd);	
}
	
void DirectedGraph::addEdge(size_t srcInd, size_t destInd){		
	setCell(srcInd,destInd,outEdges);
	setCell(destInd,srcInd,inEdges);	
	idxToOut[srcInd]->set(destInd,true);
	idxToIn[destInd]->set(srcInd,true);
}

void DirectedGraph::removeEdge(varType src, varType dest){
	clearCell(src,dest,outEdges);
	clearCell(dest,src,inEdges);
}

/*
	returns the set of nodes that can reach any one of the nodes in vars
	
unordered_map<varType,varSet> DirectedGraph::havePathTo(const varSet& vars){
	this->transpose();
	DFS dfs(*this);
	dfs.runDFSFromVars(vars);
	this->transpose();
	return dfs.getDFSTreeMap();
	
}*/

void DirectedGraph::removeVarConnectInOut(varType v){
	NbsIt nbsIt(v,*this);
	while(nbsIt.hasNextIn()){
		varType vInNb = nbsIt.nextIn();
		if(vInNb==v) continue;
		nbsIt.resetOut();
		while(nbsIt.hasNextOut()){
			varType vOutNb = nbsIt.nextOut();
			if(vOutNb == v) continue;
			addEdge(vInNb, vOutNb);
		}
	}
	removeVar(v);
}

void DirectedGraph::removeVar(varType v){
	size_t vInd=varToIndex.at(v);
	size_t begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? inEdges.find_next(begIn-1) : inEdges.find_first();
	size_t end = getIndex(vInd,innerSize);
	//run over all indexes inNbsIdx entering v, clear the entry outEdges[inNbsIdx,v]
	for(size_t inNbsIdx=begIn ; inNbsIdx < end &&  inNbsIdx != boost::dynamic_bitset<>::npos; inNbsIdx = inEdges.find_next(inNbsIdx) ){
		size_t idx = getIndex(inNbsIdx % innerSize, vInd);
		outEdges.reset(idx);
		//outEdges[idx]=false;			
	}
	begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? outEdges.find_next(begIn-1) : outEdges.find_first();
	//run over all indexes outNbsIdx going leaving v, clear the entry inEdges[outNbsIdx,v]
	for(size_t outNbsIdx= begIn ; outNbsIdx < end &&  outNbsIdx != boost::dynamic_bitset<>::npos; outNbsIdx = outEdges.find_next(outNbsIdx) ){
		size_t idx = getIndex(outNbsIdx  % innerSize, vInd);
		inEdges.reset(idx);
		//inEdges[idx]=false;		
	}	
	begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? inEdges.find_next(begIn-1) : inEdges.find_first();
	for(size_t inNbsIdx=begIn ; inNbsIdx < end &&  inNbsIdx != boost::dynamic_bitset<>::npos; inNbsIdx = inEdges.find_next(inNbsIdx) ){
		inEdges.reset(inNbsIdx);
		//inEdges[inNbsIdx]=false;		
	}
	begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? outEdges.find_next(begIn-1) : outEdges.find_first();
	for(size_t outNbsIdx=begIn ; outNbsIdx < end &&  outNbsIdx != boost::dynamic_bitset<>::npos; outNbsIdx = outEdges.find_next(outNbsIdx) ){
		outEdges.reset(outNbsIdx);
		//outEdges[outNbsIdx]=false;		
	}
	//varToIndex.erase(v);
	vertices.erase(v);
}

void DirectedGraph::removeVars(const varSet& vars){
	varSet::const_iterator end = vars.cend();
	for(varSet::const_iterator vit = vars.cbegin() ; vit != end ; ++vit){
		removeVar(*vit);
	}
}

void DirectedGraph::removeVarSetConnectInOut(const varSet& vars){
	for(varSet::const_iterator cit = vars.cbegin() ; cit != vars.cend() ; ++cit){
		removeVarConnectInOut(*cit);
	}
}

void DirectedGraph::transpose(){
	DBS temp = inEdges;
	inEdges = outEdges;
	outEdges = temp;	
}

DirectedGraph::DirectedGraph(const DirectedGraph& other):vertices(other.vertices.cbegin(),other.vertices.cend()),varToIndex(other.varToIndex){		
	inEdges = other.inEdges;
	outEdges = other.outEdges;	
	this->indexToVar = other.indexToVar;
	this->innerSize = other.innerSize;	
	idxToOut = NULL;
	idxToIn = NULL;
}

long DirectedGraph::timeToCalcDegree=0;
int DirectedGraph::inDegree(varType v){	
	size_t vInd=varToIndex.at(v);
	size_t begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? inEdges.find_next(begIn-1) : inEdges.find_first();
	size_t end = getIndex(vInd,innerSize);
	//run over all indexes inNbsIdx entering v, clear the entry outEdges[inNbsIdx,v]
	int inDeg = 0;
	for(size_t inNbsIdx=begIn ; inNbsIdx < end &&  inNbsIdx != boost::dynamic_bitset<>::npos; inNbsIdx = inEdges.find_next(inNbsIdx) ){
		++inDeg;
	}	
	return inDeg;
}
int DirectedGraph::outDegree(varType v){	
	size_t vInd=varToIndex.at(v);
	size_t begIn = getIndex(vInd,0);
	begIn = (begIn > 0) ? outEdges.find_next(begIn-1) : outEdges.find_first();
	size_t end = getIndex(vInd,innerSize);
	//run over all indexes inNbsIdx entering v, clear the entry outEdges[inNbsIdx,v]
	int outDeg = 0;
	for(size_t outNbsIdx=begIn ; outNbsIdx < end &&  outNbsIdx != boost::dynamic_bitset<>::npos; outNbsIdx = outEdges.find_next(outNbsIdx) ){
		++outDeg;
	}	
	return outDeg; 
}
/*
bool DirectedGraph::topologicalSort(list<varType>& retVal){
	varSet empty;
	return topologicalSort(retVal, empty);
}
*/
/*
bool DirectedGraph::topologicalSort(list<varType>& retVal,const varSet& priorSet){
	DirectedGraph tempG(*this);
	varSet priorSetClosure(priorSet);
	calculateSetClosure(priorSetClosure);

	list<varType> in0Queue;
	for(varSet::const_iterator cit = tempG.vertices.cbegin() ; cit !=  tempG.vertices.cend() ; ++cit){
		if(inDegree(*cit) == 0){
			if(priorSetClosure.find(*cit) != priorSetClosure.end()){
				in0Queue.push_front(*cit);
			}
			else{
				in0Queue.push_back(*cit);
			}
		}
	}
	while(!in0Queue.empty()){
		varType v = in0Queue.front();
		in0Queue.pop_front();
		retVal.push_back(v);
		NbsIt vNbs(v,tempG);		
		//run over all indexes leaving v	
		while(vNbs.hasNextOut()){
			varType vOutNb = vNbs.nextOut();		
			tempG.removeEdge(v,vOutNb);
			if(tempG.inDegree(vOutNb) == 0){
				if(priorSetClosure.find(vOutNb) != priorSetClosure.end()){
					in0Queue.push_front(vOutNb);
				}
				else{
					in0Queue.push_back(vOutNb);
				}
			}
		}
		tempG.removeVar(v);
	}

	if(tempG.isEmpty())
		return true;

	retVal.clear();
	return false;
}*/

	bool DirectedGraph::selfLoop(varType v){
		size_t vInd = varToIndex.at(v);
		return outEdges[getIndex(vInd,vInd)];

	}

	const varSet DirectedGraph::computeFVS() const{
		varSet FVS;
		//reduction methods are made on a copy of the graph
		DirectedGraph G_reduce(*this);
		while(!G_reduce.isEmpty()){
			G_reduce.reduce_5_Ops(FVS);
			if(G_reduce.isEmpty()){
				break;
			}
			//use PIE reductions
			DirectedGraph G_reduceMinusPIE(G_reduce); //copy of current reduced graph G_reduce
			bool PIEReductions = false;
			get_GMinusPIE(G_reduceMinusPIE); //now, it does not contain 2-cycles
		//	PIEReductions = G_reduce.reduce_PIE(G_reduceMinusPIE); //reduce by PIE
			PIEReductions |= G_reduce.reduce_DOME(G_reduceMinusPIE); //reduce by DOME
			if(PIEReductions)
				continue;
			//if here, the reduction did not change the graph -- > we must eliminate a vertex
			G_reduce.eliminateVertex(FVS);
		}
		return FVS;
	}
	/*
	reductions based on the paper "An optimal algorithm for cycle breaking in directed graphs" by Orenstein,Kohavi and Pomeranz
	*/
	bool DirectedGraph::reduce_5_Ops(varSet& FVS){		
		bool changed = true;	
		int numOfIterations;
		for(numOfIterations = 0; (!isEmpty() && changed) ; numOfIterations++){	//continue only while there is at least one variable eliminated during each iteration	
			varSet verticesCpy(vertices); //so we can iterate while modifying the vertices structure
			changed = false;
			for(varSet::iterator vit = verticesCpy.begin(); vit  != verticesCpy.end() ; ++vit){				
				if(selfLoop(*vit)){
					removeVar(*vit);
					FVS.insert(*vit);
					changed = true;
					continue;
				}
				if(inDegree(*vit) == 0){
					removeVar(*vit);
					changed = true;
					continue;
				}
				if(outDegree(*vit) == 0){
					removeVar(*vit);
					changed = true;
					continue;
				}
				if(inDegree(*vit) == 1){
					removeVarConnectInOut(*vit);
					changed = true;
					continue;
				}
				if(outDegree(*vit) == 1){
					removeVarConnectInOut(*vit);
					changed = true;
					continue;
				}			
			}			
		}
		return (numOfIterations>1); //if it is 1, then only a single iteration ran which means that changed = false after a single iteration --> no changes were made to the graph
	}

	/*
	From "Markov-Chain-Based Heuristics for the Feedback Vertex Set Problem for Digraphs"
	PIE:={(u,v) \in A: (v,u) \in A}
	B:=A_{acyc}(G-PIE), meaning all of the arcs that are not part of any Strongly Connected Component in G-PIE.
	G is transformed into G-B
	returns true if the graph has changed
	
	bool DirectedGraph::reduce_PIE(DirectedGraph& GMinusPIE){
		bool changed = false;		
		list<varSet> MSCs;
		unordered_map<varType,varType> varToMSCId;	
		DFS::getMSCs(GMinusPIE,MSCs,varToMSCId);
		const varSet& vars = getVertices();
		//now iterate over the edges and remove those that do not belong to PIE or to any MSC
		varSet::const_iterator end =  vars.cend();
		for(varSet::const_iterator vit = vars.cbegin() ; vit != end ; ++vit){
			varType v = *vit;
			NbsIt vNbs(v,*this);			
			while(vNbs.hasNextOut()){
				varType vOutNb = vNbs.nextOut();			
				if(varToMSCId[v] != varToMSCId[vOutNb]){ //edge *vit --> *out_v_it is between two different connected components in G-PIE
					if(!is2Cycle(v,vOutNb)){ //if edge is in PIE, do not remove it						
						removeEdge(v, vOutNb);
						changed = true;
					}
				}
			}			
		}
		return changed;
	}
	*/
	const DBS& DirectedGraph::getIncomingBSInd(size_t vInd){
		return *idxToIn[vInd];		
	}
	

	const DBS& DirectedGraph::getIncomingBS(varType v){		
		size_t vInd = varToIndex.at(v);
		return getIncomingBSInd(vInd);		
	}

	const DBS& DirectedGraph::getOutgoingBSInd(size_t vInd){
		return *idxToOut[vInd];		
	}

	const DBS& DirectedGraph::getOutgoingBS(varType v){		
		size_t vInd = varToIndex.at(v);
		return getOutgoingBSInd(vInd);		
	}

	void DirectedGraph::getOutNbs(varType v, DBS& retVal){
		NbsIt vNbsIt(v,*this);
		while(vNbsIt.hasNextOut()){			
			retVal.set(vNbsIt.nextOut(),true);			
		}
	}
	void DirectedGraph::getInNbs(varType v, DBS& retVal){
		NbsIt vNbsIt(v,*this);
		while(vNbsIt.hasNextIn()){
			retVal.set(vNbsIt.nextIn(),true);			
		}
	}

	void DirectedGraph::getOutNbs(varType v, varSet& retVal) {
		NbsIt vNbsIt(v,*this);
		while(vNbsIt.hasNextOut()){
			retVal.insert(vNbsIt.nextOut());
		}
	}

	void DirectedGraph::getInNbs(varType v, varSet& retVal) {
		NbsIt vNbsIt(v,*this);
		while(vNbsIt.hasNextIn()){
			retVal.insert(vNbsIt.nextIn());
		}
	}
	/*
	From "Markov-Chain-Based Heuristics for the Feedback Vertex Set Problem for Digraphs"
	PIE:={(u,v) \in A: (v,u) \in A}
	remove dominated arcs using this rule:
	(u,v) is dominated if N^-_{G-PIE}(u) \subset  N^-_{G}(u) or N^+_{G-PIE}(v) \subset  N^+_{G}(u)
	*/
	bool DirectedGraph::reduce_DOME(DirectedGraph& GMinusPIE){
		bool changed = false;
		const varSet& vars = getVertices();
		//now iterate over the edges and remove those that are dominated
		for(varSet::const_iterator vit = vars.cbegin() ; vit != vars.cend() ; ++vit){
			varType u = *vit;
			NbsIt uNbsIt(u,*this);
			//varSet out_v = getOutgoingEdges(*vit);	//must have outgoing edges...			
			while(uNbsIt.hasNextOut()){			
				varType v=uNbsIt.nextOut();
				if(!is2Cycle(u,v)){  //check for domination
					DBS u_GMinusPIE_in = GMinusPIE.getIncomingBS(u);
					DBS v_GMinusPIE_out = GMinusPIE.getOutgoingBS(v);
					DBS v_G_in = getIncomingBS(v);
					DBS u_G_out = getOutgoingBS(u);
					if(u_GMinusPIE_in.none() || v_GMinusPIE_out.none()){ //in this case GMinusPIE.incomingEdges.find(u) \subset v_G_in since it cannot have degree 0							
							removeEdge(u,v);
							changed = true;
							continue;
					}					
					if(u_GMinusPIE_in.is_proper_subset_of(v_G_in)){	//Utils::properContains(v_G_in,u_GMinusPIE_in)
						removeEdge(u,v);
						changed = true;
						continue;
					}
					if(v_GMinusPIE_out.is_proper_subset_of(u_G_out)){//Utils::properContains(u_G_out,v_GMinusPIE_out)
						removeEdge(u,v);
						changed = true;
						continue;
					}
				}			
			}
		}
		return changed;
	}
	/*
	Change G such that it does not contain any cycles of length 2.
	*/
	void DirectedGraph::get_GMinusPIE(DirectedGraph& G){
		//remove edges that form cycles of length 2.
		const varSet& Gvars = G.getVertices();
		for(varSet::const_iterator vit = Gvars.cbegin() ; vit != Gvars.cend() ; ++vit){
			NbsIt nbsIt(*vit,G);
			while(nbsIt.hasNextOut()){
				varType vOutNb = nbsIt.nextOut();
				if(G.isEdge(vOutNb,*vit)){ //two cycle					
					G.removeEdge(vOutNb,*vit);
					G.removeEdge(*vit,vOutNb);
				}			
			}			
		}
	}

	bool DirectedGraph::isEdge(varType v, varType u) const{
		size_t vInd = varToIndex.at(v);
		size_t uInd = varToIndex.at(u);
		return outEdges[getIndex(vInd,uInd)];
	}

	bool DirectedGraph::isEdge(size_t vInd, size_t uInd) const{
		return outEdges[getIndex(vInd,uInd)];
	}
	//returns true if there is a cycle u->v->u
	bool DirectedGraph::is2Cycle(varType v, varType u){
		return (isEdge(v,u) && isEdge(u,v));
	}

	/*
	Select a vertex for elimination and insertion into the FVS.
	Heuristic used: select the vertex with MAX(In(v)+Out(v))
	*/
	void DirectedGraph::eliminateVertex(varSet& FVS){
		int maxDeg = 0;
		varType maxVar=0;
		varSet::const_iterator end = vertices.cend();
		for(varSet::const_iterator vit = vertices.cbegin() ; vit != end ; ++vit){
			//int currVarIn = incomingEdges[*vit].size();
			int currVarIn = inDegree(*vit);
			//int currVarOut = outgoingEdges[*vit].size();
			int currVarOut = outDegree(*vit);
			if(maxDeg < (currVarIn + currVarOut)){
				maxDeg = (currVarIn + currVarOut);
				maxVar = *vit;
			}
		}
		FVS.insert(maxVar);
		removeVar(maxVar);
	}

	void DirectedGraph::removeOutgoingEdges(varType v){		
		size_t vInd=varToIndex.at(v);
		size_t begIn = getIndex(vInd,0);
		begIn = (begIn > 0) ? outEdges.find_next(begIn-1) : outEdges.find_first();
		size_t end = getIndex(vInd,innerSize);
		//run over all indexes leaving v	
		for(size_t outNbsIdx=begIn; outNbsIdx < end &&  outNbsIdx != boost::dynamic_bitset<>::npos; outNbsIdx = outEdges.find_next(outNbsIdx) ){
			removeEdge(v,indexToVar[outNbsIdx % innerSize]);			
		}

	}

	void DirectedGraph::clearAllEdges(){
		inEdges.reset();
		outEdges.reset();
	}

	string DirectedGraph::printGraph() const{
		std::stringstream ss;	
		ss << "The participating variables: "<<  Utils::printSet(vertices) << endl;		
		ss << "outgoing edges: " << endl;		
		DirectedGraph copyMe(*this);
		for(varSet::const_iterator varIt = vertices.cbegin() ; varIt != vertices.cend() ; ++varIt){
			ss << *varIt << ": " ;
			NbsIt nbsIt(*varIt,copyMe);
			while(nbsIt.hasNextOut()){
				ss << nbsIt.nextOut() << ",";
			}
			ss << endl;
		}
		ss << endl;
		ss << "incoming edges: " << endl;
		for(varSet::const_iterator varIt = vertices.cbegin() ; varIt != vertices.cend() ; ++varIt){
			ss << *varIt << ": " ;
			NbsIt nbsIt(*varIt,copyMe);
			while(nbsIt.hasNextIn()){
				ss << nbsIt.nextIn() << ",";
			}
			ss << endl;
		}

		return ss.str();	
	}


	/*
	updates set to contain all of its ancestors in the graph
	*/
	/*
	void DirectedGraph::calculateSetClosure(varSet& set){
		if(set.empty()) return;			

		DirectedGraph cpy(*this);
		cpy.transpose();
		DFS dfsExec(cpy);
		dfsExec.runDFSFromVars(set);
		varSet ancestors;

		boost::unordered_map<varType,varSet>& dfsTreeMap =  dfsExec.getDFSTreeMap();
		for(varSet::const_iterator varIt = set.cbegin() ; varIt != set.cend() ; ++varIt){
			if(dfsTreeMap.find(*varIt) != dfsTreeMap.end()){
				varSet& varTree = dfsTreeMap[*varIt];
				ancestors.insert(varTree.cbegin(), varTree.cend());
			}
		}

		set.insert(ancestors.cbegin(), ancestors.cend());
	}
	*/
	DirectedGraph::~DirectedGraph(){
		outEdges.reset();
		inEdges.reset();
		if(idxToOut != NULL){
			for(int i=0 ; i < (innerSize+1) ; i++){
				delete(idxToOut[i]);
				idxToOut[i] = NULL;
			}
			delete[] idxToOut;
		}
		
		if(idxToIn != NULL){
			for(int i=0 ; i < (innerSize+1) ; i++){
				delete(idxToIn[i]);
				idxToIn[i] = NULL;
			}
			delete[] idxToIn;
		}
		
	}
	/*
	void DirectedGraph::test(){
		varSet vars;
		vars.insert(1);vars.insert(2);vars.insert(3);vars.insert(4);
		vars.insert(5);vars.insert(6);vars.insert(7);vars.insert(8);vars.insert(9);vars.insert(10);
		DirectedGraph D(vars);
		D.addEdge(1,2);D.addEdge(1,7); D.addEdge(1,9); 
		D.addEdge(2,3);D.addEdge(2,5);
		D.addEdge(3,4);
		D.addEdge(4,1);
		D.addEdge(5,6);
		D.addEdge(6,2);D.addEdge(6,7);
		D.addEdge(7,5);D.addEdge(7,10);
		D.addEdge(8,3);D.addEdge(8,4);
		D.addEdge(9,1);
		cout << "The originl (cyclic) graph: " << endl << D.printGraph();
		list<varType> topSort;
		bool isSorted = D.topologicalSort(topSort);
		cout << "topologicalSort on the cyclic graph returned: " << isSorted << endl;
			
		varSet FVS = D.computeFVS();
		cout << " FVS for the graph is : " << Utils::printSet(FVS);

		topSort.clear();
		DirectedGraph DCopy(D);
		DCopy.removeVars(FVS);
		cout << "The graph after removing FVS: " << endl << DCopy.printGraph();
		isSorted = DCopy.topologicalSort(topSort);
		cout << "topologicalSort on DCopy returned: " << isSorted << "  sort order: " << Utils::printList(topSort) << endl;

		DirectedGraph DCopy2(D);
		for(varSet::const_iterator vit = FVS.cbegin() ; vit != FVS.cend() ; ++vit){
			DCopy2.removeOutgoingEdges(*vit);
		}
		cout << "The graph after removing FVS outgoing edges: " << endl << DCopy2.printGraph();
		topSort.clear();
		isSorted = DCopy2.topologicalSort(topSort,FVS);
		cout << "topologicalSort on DCopy2 returned: " << isSorted << "  sort order: " << Utils::printList(topSort) << endl;

		varSet subGraphVars;
		subGraphVars.insert(1); subGraphVars.insert(2); subGraphVars.insert(10); subGraphVars.insert(11); 
		subGraphVars.insert(12); subGraphVars.insert(13); 
		DirectedGraph subGraph(subGraphVars);
		subGraph.addEdge(11,10); 
		subGraph.addEdge(10,2);
		subGraph.addEdge(2,12);subGraph.addEdge(2,13);
		subGraph.addEdge(12,13);subGraph.addEdge(12,1);
		subGraph.addEdge(13,1);
		cout << "subGraph after creation: " << endl << subGraph.printGraph();

		//test
		DirectedGraph cpy(subGraph);
		cout << "cpy: " << endl << cpy.printGraph();
		cpy.transpose();
		cout << "cpy-transpose: " << endl << cpy.printGraph();

		
		varSet varsToRemove;
		varsToRemove.insert(11); varsToRemove.insert(12); varsToRemove.insert(13); 
		subGraph.removeVarSetConnectInOut(varsToRemove);

		cout << "subGraph after projecting out vertices 11-13: " << endl << subGraph.printGraph();

//		DCopy2.addEdges(subGraph);
		cout << " The graph DCopy2 after adding the subgraph edges: " << endl << DCopy2.printGraph();
		topSort.clear();
		isSorted = DCopy2.topologicalSort(topSort);
		cout << "topologicalSort on DCopy2 (after adding subgraph edges) returned: " << isSorted << "  sort order: " << Utils::printList(topSort) << endl;

		varSet varPIE;
		varPIE.insert(1); varPIE.insert(2); varPIE.insert(3); varPIE.insert(4); 
		varPIE.insert(5); 
		DirectedGraph DPie(varPIE);
		DPie.addEdge(1,2);DPie.addEdge(1,3);
		DPie.addEdge(2,3);DPie.addEdge(2,4);DPie.addEdge(2,5);
		DPie.addEdge(3,2);DPie.addEdge(3,1);
		DPie.addEdge(4,3);DPie.addEdge(4,5);
		DPie.addEdge(5,1);DPie.addEdge(5,2);DPie.addEdge(5,3);DPie.addEdge(5,4);
		
		
		topSort.clear();
		isSorted = DPie.topologicalSort(topSort);
		cout << "topologicalSort on DPie returned: " << isSorted ;
		varSet pieFVS = DPie.computeFVS();

		cout << " FVS for the graph is : " << Utils::printSet(pieFVS);
			for(varSet::const_iterator vit = pieFVS.cbegin() ; vit != pieFVS.cend() ; ++vit){
			DPie.removeOutgoingEdges(*vit);
		}
		cout << " The graph DPie after removing FVS outgoing edges: " << endl << DPie.printGraph();
		topSort.clear();
		isSorted = DPie.topologicalSort(topSort,pieFVS);
		cout << "topologicalSort on DPie returned: " << isSorted << "  sort order: " << Utils::printList(topSort) << endl;
	}
	*/