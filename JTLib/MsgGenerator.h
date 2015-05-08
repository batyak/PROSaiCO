#ifndef MSGGEN_H
#define MSGGEN_H

#include "Assignment.h"
#include "DirectedGraph.h"
#include "TreeNode.h"



class MSGGenerator{
public:
	static size_t totalNumOfEntries;
	static size_t cacheHits;
	static size_t numOfBTs;
	static size_t numOfPrunes;

	MSGGenerator(TreeNode* TN_);	
	virtual probType buildMsg(Assignment& context, probType lb,DBS& zeroedClauses, Assignment* bestAssignment=NULL)=0;	
	virtual void initByTN()=0;
	virtual void updateBestAssignment(Assignment& context, DBS& zeroedClauses, Assignment& MPEAss)=0;
	virtual const DBS& getTermsWithLit(varType lit) ;

	size_t getFactorSize() const{
		return factorSize;
	}

	size_t getZeroedEntries() const{
		return zeroedEntries;
	}

	bool allVarClausesZero(varType v, const DBS& zeroedTerms);
	bool varTermsIsolated(varType v, const DBS& zeroedTerms);
	virtual ~MSGGenerator(){
		if(compressedToReg != NULL){
			delete(compressedToReg);
		}
		if(RegToCompressed != NULL){
			delete(RegToCompressed);
		}
	}
	//bool isTermsSetIsolated(const DBS& termSet, const list<TreeNode*>& relevantChildren) const;
	/**
	Check if the termSet, all containing v is isolated
	*/
	bool isTermsSetIsolated(const DBS& termSet, varType v) const;
protected:
	TreeNode* TN;
	FormulaMgr& FM;	
	virtual void calculateZeroedClauses(const Assignment& currEntry, DBS& zeroedClauses);
	//internal treNode messages which need to be called
	//delegate calls to private/protected methods
	void updateNodeLevel();
	void updateGlobalAssignment(const Assignment& GA_);
	void getRelevantMembers(const Assignment& GA, varSet& relevantVars, const DBS& zeroedTerms);
//	DirectedGraph* getRelevantGraph(const varSet& relevantVars,const DBS& zeroedTerms);
	DirectedGraph* getRelevantGraphNew(const varSet& relevantVars,const DBS& zeroedTerms);

	
	
	size_t factorSize; //num of distinct entries in parent relevant to this node
	size_t zeroedEntries;
	
private:
	varToBS litToSubForm;
	varToBS litToCompressedSubForm; //isntead of a sparse long BS, the bitsets here will have the size of the subformula
	size_t* compressedToReg;	
	size_t* RegToCompressed;	
	size_t numOfTNSubTerms;

	const DBS& getCompressedTermsWithLit(varType lit) ;
	void translateFromCompressed(const DBS& compressed, DBS& trans) const;
	void translateToCompressed(const DBS& terms, DBS& compressed) const;
	void getChildrenWithVar(varType v, list<TreeNode*>& vChildren) const;
};










#endif
