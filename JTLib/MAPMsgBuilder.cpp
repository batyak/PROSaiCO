#include "MAPMsgBuilder.h"

probType MAPMsgBuilder::getDNFTermMPProb(const Assignment& context,const DBS& irrelevantVars ,Assignment* MPEAss) const{
	if(!TN->isDNFTerm()){ //not  DNF term,cannot be satisfied
		return 1.0;
	}	
	varType varToSet=0;
	const varSet& vars = TN->getVars();
	varSet::const_iterator end = vars.end();
	probType MPEProb = 1.0;	
	probType maxZeroOneRatio = 0; //i.e., MAX[p(l^)/p(l)]
	bool isZeroed = false;
	probType SATProb = 1.0;
	for(varSet::const_iterator it = vars.begin() ; it != end ; ++it){
		varType v = *it;
		Status vStat = context.getStatus(v);
		if(vStat != UNSET){
			isZeroed = isZeroed || (vStat==SET_F);  //found a literal assigned 0
			continue;
		}
		if(!FM.isMPVar(v)){ //a SUM var
			SATProb*=FM.getLitProb(v);
		}
		else{
			bool irrelevant = !irrelevantVars.empty() && irrelevantVars[(v > 0) ? v : -v];
			if(irrelevant)
				continue; //will be set in a different branch - if here, means that this term MUST be zeoed our
			//if here, v is an unassigned relevant MPE variable
			probType posProb = FM.getLitProb(v);
			probType negProb = FM.getLitProb(0-v);
			if(negProb >= posProb){ //better for the literal to be assigned 0
				MPEProb*=negProb;
				isZeroed = true;	
				if(MPEAss != NULL){
					MPEAss->setVar(v,false);
				}
			}
			else{ //here  posProb > 0
				MPEProb*=posProb;
				probType ratio = (negProb/posProb); //in case all MAP vars are assigned 1
				if(ratio > maxZeroOneRatio){
					maxZeroOneRatio = ratio;
					varToSet = v;
				}
				if(MPEAss != NULL){
					MPEAss->setVar(v,true);
				}			
			}
		}
	
	}	
	//DEBUG
	//string ctxtStr = context.print();
	
	if(isZeroed){
		return MPEProb; //either the evidence or the MPE assignment already zero this term --> return max probability
	}
	//now choose best option between:
	//1. setting one of the SUM vars to 0 (1.0-satProb)*MPEProb 
	//2. Making sure one of the MPEvars is 0 - MPEProb*maxZeroOneRatio
	probType op1 = (1.0-SATProb)*MPEProb; //if SATProb==1, then this will 0-out
	probType op2 = MPEProb*maxZeroOneRatio;
	if(op1 >= op2){
		return op1; //took into account that one of the SUM vars is 0
	}
	if(MPEAss != NULL && varToSet != 0){
		MPEAss->setVar(varToSet,false);
	}	
	return op2;
}


probType MAPMsgBuilder::OptimalAssignmentProb(const DBS& unassignedVars,Assignment* optimalAss) const{
	DBS unassignedMPVars(unassignedVars);
	unassignedMPVars.operator&=(FM.getMPVars());
	return MPEMsgBuilder::OptimalAssignmentProb(unassignedMPVars,optimalAss);
}


probType MAPMsgBuilder::updateIrrelevantVars(const DBS& zereodTerms, DBS& irrelevantVars, Assignment& context){
	probType retProb = 1.0;
	for(size_t v = irrelevantVars.find_first() ; v != DBS::npos ; v = irrelevantVars.find_next(v)){
		if(!allVarClausesZero((varType)v,zereodTerms)){ //means that the variable is relevant (appears in at least one subformula)
			irrelevantVars[v] = false;
		}		
		else{ //do nothing for irrelevant SUM vars
			if(FM.isMPVar((varType)v)){
				const DBS& parentVars = (TN->getParent() != NULL) ? TN->getParent()->getVarsBS() : TreeNode::emptyDBS;
				/*
				if parentVars[v]: This means that:
				1. v is irrelevant to the current subformula
				2. v is relevant for the parent
				--> v must be relevant to one of TN's siblings --> disregard it --> mark as irrelevant but do not assign
				*/			
				if(parentVars.empty() || !parentVars[v]){ //v is not part of the parent --> cannot be part of a sibling formula --> can assign		
					irrelevantVars[v] = false;
					varType var_v = (varType) v;
					/*
					This means that:
					1. v is irrelevant to the current subformula
					2. v does not belong to the parent --> it cannot belong to an of TN's siblings
					--> v is irrelevant in this subformula && does not appear in any siblong subformula -->
					we can assign it the optimal value.
					irrelevantVars[v] = false; //only mark as 
					*/
					probType vPos = FM.getLitProb(var_v);
					probType vNeg = FM.getLitProb(-var_v);
					if(vNeg >= vPos){
						retProb*=vNeg;
						context.setVar(var_v,false);
					}
					else{
						retProb*=vPos;
						context.setVar(var_v,true);
					}
				}
			}
		}
	}
	return retProb;
}
/*
probType MAPMsgBuilder::updateAssignmentWithIrrelevantVars(const DBS& zereodTerms, const DBS& irrelevantVars,Assignment& context){
	DBS unassignedMPIrrelevantVars(irrelevantVars);
	unassignedMPIrrelevantVars.operator&=(FM.getMPVars());
	return MPEMsgBuilder::updateAssignmentWithIrrelevantVars(zereodTerms,unassignedMPIrrelevantVars,context);
}*/