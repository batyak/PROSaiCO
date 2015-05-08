#include "Calculator.h"
#include <limits>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/expm1.hpp>
using namespace boost::math; 

probType Calculator::addNormal(probType p1, probType p2){
	return (p1+p2);
}

probType Calculator::multNormal(probType p1, probType p2){
	return (p1*p2);
}

probType Calculator::multLOGE(probType p1, probType p2){
	return (p1+p2);
}

probType Calculator::addLOGE(probType p1, probType p2){
	// We are in log space, which makes things complicated.  First, ensure that
	// p1 is the larger.
	if (p2 > p1) {
	    double temp = p1;
	    p1 = p2;
	    p2 = temp;
	}
	// If the smaller is 0, then return the larger.
	if (p2 == -std::numeric_limits<probType>::infinity()) {return p1;}
	// If the smaller number is twenty orders of magnitude smaller, then ignore
	// the smaller.
	probType negDiff = p2 - p1;
	if (negDiff < -1000.0) {return p1;}	
	return p1 + log1p(expm1(negDiff) + 1.0);
	    
}