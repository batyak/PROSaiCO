#include "ConfigurationIterator.h"
#include "Params.h"

long configuration_iterator::timeToIncrement=0;
DirectedGraph configuration_iterator::emptyGraph;

configuration_iterator configuration_iterator::begin(DirectedGraph* relevantOCG,
		probType leafProb,SubFormulaIF& sf, probType bound){
	if(relevantOCG != NULL){
		return configuration_iterator(*relevantOCG,sf, bound);
	}
	return configuration_iterator(leafProb,sf, bound);
}

configuration_iterator configuration_iterator::end(SubFormulaIF& sf){
	return configuration_iterator(sf);
}