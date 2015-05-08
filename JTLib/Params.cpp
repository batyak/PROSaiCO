#include "Params.h"

Params* Params::inst = NULL;
Params& Params::instance(){
	if(inst == NULL){
		inst = new Params();
	}
	return *inst;
}

Params* Params::x_instance(){
	if(inst == NULL){
		inst = new Params();
	}
	return inst;
}
