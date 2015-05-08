#ifndef DEFS_H
#define DEFS_H
#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS
#include "boost/dynamic_bitset.hpp"
#include "boost/unordered_set.hpp"
#include "boost/unordered_map.hpp"

typedef signed int int32;
typedef float float4;
typedef int32 	varType;
typedef double 	probType;

typedef boost::unordered_set<varType> varSet;
//typedef unsigned int nodeId;	
typedef size_t nodeId;	
typedef unsigned int dlevel;
typedef boost::unordered_set<nodeId> nodeIdSet;	
typedef boost::unordered_map<varType,bool> varToVal;
typedef boost::dynamic_bitset<> DBS;


struct varInfo{
	nodeIdSet containingTerms;
	varSet clique;
	int PEOIndex;		
};


typedef boost::unordered_map<varType,probType> varToProbMap;
typedef boost::unordered_map<varType,nodeIdSet> varToNodeIdSet;
typedef boost::unordered_map<varType,DBS> varToDNFTerms;
typedef boost::unordered_map<varType,DBS> varToBS;
typedef boost::unordered_map<varType,varInfo> varToContainingTermsMap;
typedef boost::unordered_map<nodeId, nodeIdSet> EdgesMap;
typedef boost::unordered_map<varType,size_t> varToIndexMap;
typedef boost::unordered_map<varType,nodeIdSet> varToTreeNodeMap;
typedef boost::unordered_map<varType, bool> varAssignment;

typedef boost::unordered_map<unsigned long,probType> longToProbMap;

enum Status
{
    UNSET,
    SET_T,
    SET_F
};


#endif
