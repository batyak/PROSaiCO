#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "Definitions.h"
class Calculator{
public:
	virtual probType add(probType p1, probType p2) =0;
	virtual probType mult(probType p1, probType p2) =0;
protected:
	Calculator(){}
	static probType addNormal(probType p1, probType p2);
	static probType multNormal(probType p1, probType p2);
	static probType addLOGE(probType p1, probType p2);
	static probType multLOGE(probType p1, probType p2);

};







#endif