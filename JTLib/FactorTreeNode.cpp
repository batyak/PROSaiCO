#include "FactorTreeNode.h"
#include "Params.h"

DirectedGraph FactorTreeInternal::emptyGraph;
Assignment* FactorTreeNode::emptyAss = NULL;
dlevel FactorTreeInternal::currAssignmentlevel = 0;
DBS FactorTreeNode::emptyDBS;
bool FactorTreeNode::minCostSAT = Params::instance().ADD_LIT_WEIGHTS;
dlevel FactorTreeInternal::getNextAsssignmentLevel(){
	return ++FactorTreeInternal::currAssignmentlevel;
}

DBS FactorTreeInternal::vInBS;
DBS FactorTreeInternal::v_InBS;
DBS FactorTreeInternal::vTotalIn;
void FactorTreeInternal::setUpScoreCompDBS(const DirectedGraph& nodeOCG){
	size_t v_size = nodeOCG.getVertices().size();
	if(vInBS.size() != v_size){
		vInBS.resize(v_size);
		v_InBS.resize(v_size);
		vTotalIn.resize(v_size);
	}
}