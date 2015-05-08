#ifndef DIRECTED_GRAPH_H
#define DIRECTED_GRAPH_H


#include <iostream>
#include "boost/dynamic_bitset.hpp"
#include <vector>
#include <string>
#include <list>
#include "Definitions.h"
using namespace boost;



class DirectedGraph{
	//typedef boost::unordered_map<size_t,DBS> idxToInOutEdges; //from var index to bit set of in/outgoing edges
	typedef std::vector<DBS> idxToInOutEdges; //from var index to bit set of in/outgoing edges

//	friend class DFS;	
	friend class NbsIt;
public:
	static long timeToCalcDegree;
	DirectedGraph(){
		innerSize = 0;
		idxToOut = NULL;
		idxToIn=NULL;
	}
	DirectedGraph(const varSet& vertices):vertices(vertices){
		initGraphStructures();	
	}

	DirectedGraph(const DirectedGraph& other);
	DirectedGraph* getSubgraph(const varSet& sub_vertices);	
	virtual ~DirectedGraph();
	void addEdge(varType src, varType dest);	
	void addEdge(size_t src, size_t dest);	
	void removeEdge(varType src, varType dest);
	void removeVarConnectInOut(varType v);
	void removeVar(varType v);
	void removeVars(const varSet& vars);
	void removeVarSetConnectInOut(const varSet& vars);
	void transpose(); //turns G to its transposed.
	const varSet& getVertices() const{
		return vertices;
	}

	const DBS& getIncomingBS(varType v) ;
	const DBS& getOutgoingBS(varType v) ;	
	varType getVarByIdx(size_t idx) const;

	//if there exists a topological sort, it returns it in retVal and retursn true
//	bool topologicalSort(std::list<varType>& retVal);
//	bool topologicalSort(std::list<varType>& retVal, const varSet& priorSet);
	bool isEmpty() const{
		return vertices.empty();
	}
	bool isEdge(varType v, varType u) const;
	bool isEdge(size_t v, size_t u) const;
	std::string printGraph() const;

	/*
	returns the set of nodes that can reach vars by a directed path in the graph
	*/
	//unordered_map<varType,varSet> havePathTo(const varSet& vars);
	/*
	This method changes the graph so that it contains no cycles.
	*/
	const varSet computeFVS() const;
	void removeOutgoingEdges(varType v);
	
	void clearAllEdges();
	//static void test();
	int inDegree(varType v);
	int outDegree(varType v);

	class NbsIt{
		private:
			varType v;
			DirectedGraph& DG;
			size_t currLocationIn;
			size_t currLocationOut;
			size_t vEnd;
			
		public:
			
			NbsIt(varType v, DirectedGraph& DG):v(v),DG(DG){
				size_t vInd = DG.varToIndex.at(v);
				currLocationIn = DG.getIndex(vInd,0);
				currLocationIn = (currLocationIn > 0) ? DG.inEdges.find_next(currLocationIn-1) : DG.inEdges.find_first();
				vEnd = DG.getIndex(vInd, DG.innerSize);
				currLocationOut = DG.getIndex(vInd,0);
				currLocationOut = (currLocationOut > 0) ? DG.outEdges.find_next(currLocationOut-1) : DG.outEdges.find_first();
			}

			void resetIn(){
				size_t vInd = DG.varToIndex.at(v);
				currLocationIn = DG.getIndex(vInd,0);
				currLocationIn = (currLocationIn > 0) ? DG.inEdges.find_next(currLocationIn-1) : DG.inEdges.find_first();
			}

			void resetOut(){
				size_t vInd = DG.varToIndex.at(v);
				currLocationOut = DG.getIndex(vInd,0);
				currLocationOut = (currLocationOut > 0) ? DG.outEdges.find_next(currLocationOut-1) : DG.outEdges.find_first();
			}

			bool hasNextIn(){
				return (currLocationIn < vEnd && currLocationIn != boost::dynamic_bitset<>::npos);
			}

			bool hasNextOut(){
				return (currLocationOut < vEnd && currLocationOut != boost::dynamic_bitset<>::npos);
			}

			varType nextIn(){
				size_t inNbIdx = currLocationIn % DG.innerSize;
				varType inNb = DG.indexToVar[inNbIdx];			
				currLocationIn = DG.inEdges.find_next(currLocationIn);
				return inNb;
			}

			varType nextOut(){
				size_t outNbIdx = currLocationOut % DG.innerSize;
				varType outNb = DG.indexToVar[outNbIdx];			
				currLocationOut = DG.outEdges.find_next(currLocationOut);
				return outNb;
			}
	};
	void getOutNbs(varType v, varSet& retVal) ;
	void getInNbs(varType v, varSet& retVal) ;
	void getOutNbs(varType v, DBS& retVal) ;
	void getInNbs(varType v, DBS& retVal) ;

	const size_t getVarIdx(varType v) const{
		return varToIndex.at(v);
	}
private:	
	varSet vertices;
	varToIndexMap varToIndex;
	std::vector<varType> indexToVar; 	
	size_t innerSize;
	

	DBS outEdges;
	DBS inEdges;
	DBS** idxToOut;
	DBS** idxToIn;
	void initGraphStructures();
	const DBS& getIncomingBSInd(size_t v) ;
	const DBS& getOutgoingBSInd(size_t v) ;	

	

	void setCell(size_t uInd, size_t vInd, DBS& arr){		
		size_t ind = getIndex(uInd,vInd);
		arr[ind]=true;
	}

	void clearCell(varType u, varType v, DBS& arr){
		size_t uInd = varToIndex.at(u);
		size_t vInd = varToIndex.at(v);
		size_t ind = getIndex(uInd,vInd);
		arr[ind]=false;
	}

	//returns the index corresponding to the matrix index i,j
	size_t getIndex(size_t i, size_t j) const{
		size_t arrIndex=i*(innerSize)+j;
		return arrIndex;
	}

	//returns true if there is a cycle u->v->u
	bool is2Cycle(varType v, varType u);
	//returns true if there is self loop from v to itself	
	bool selfLoop(varType v);
	/*
	reductions based on the paper "An optimal algorithm for cycle breaking in directed graphs" by Orenstein,Kohavi and Pomeranz
	1. LOOP - removes a vertex containing a self-loop
	2. IN0 -removes all vertices having in-degree 0
	3. OUT0 - removes all vertices having out-degree 0
	4. IN1 - removes all vertices having in-degree 1. We combine v and its single predecessor into a single node
	5. OUt1 - removes all vertices having out-degree 1. We combine v and its single successor into a single node
	returns true if any change was performed on the graph
	*/
	bool reduce_5_Ops(varSet& FVS);
	
	/*
	From "Markov-Chain-Based Heuristics for the Feedback Vertex Set Problem for Digraphs"
	PIE:={(u,v) \in A: (v,u) \in A}
	B:=A_{acyc}(G-PIE), meaning all of the arcs that are not part of any Strongly Connected Component in G-PIE.
	G is transformed into G-B
	returns true if graph has changed
	
	bool reduce_PIE(DirectedGraph& GMinusPIE);*/

	/*
	From "Markov-Chain-Based Heuristics for the Feedback Vertex Set Problem for Digraphs"
	PIE:={(u,v) \in A: (v,u) \in A}
	remove dominated arcs using this rule:
	(u,v) is dominated if N^-_{G-PIE}(u) \subset  N^-_{G}(u) or N^+_{G-PIE}(v) \subset  N^+_{G}(u)
	*/
	bool reduce_DOME(DirectedGraph& GMinusPIE);

	/*
	removed all 2-cycles int he graph
	*/
	static void get_GMinusPIE(DirectedGraph& G);
	
	/*
	Select a vertex for elimination and insertion into the FVS.
	Heuristic used: select the vertex with MAX(In(v)+Out(v))
	*/
	void eliminateVertex(varSet& FVS);

	/*
	updates set to contain all of its ancestors in the graph	
	*/
	//void calculateSetClosure(varSet& set);

	
};










#endif
