#include "CombinedWeight.h"
#include <math.h>

probType CombinedWeight::getWorstVal(bool ADD){
	if(!ADD){
		return 0.0;
	}
	return HUGE_VAL;
}


probType CombinedWeight::getBestVal(bool ADD){
	if(!ADD){
		return 1.0;
	}
	return 0.0;
}

probType CombinedWeight::baseVal(){
	return (Params::instance().ADD_LIT_WEIGHTS ? 0.0 : 1.0);
}
