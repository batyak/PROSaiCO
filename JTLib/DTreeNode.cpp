#include "DTreeNode.h"
#include "Utils.h"
#include "Convert.h"
#include "FormulaMgr.h"
#include <boost/algorithm/string.hpp>

DTreeNode* DTreeNode::generateFromFile(string dtreeFile, string cnfFile){
	 //assumption: CNF is simplified - has already undergone BCP, that is the c2d compiler was activated with -simplify_s
	FormulaMgr* FM = new FormulaMgr(cnfFile,false);

	std::ifstream file;
	file.open(dtreeFile.c_str());
	if (!file){
		cout << "CFG: File " + dtreeFile + " couldn't be found!\n" << endl;
		return NULL;
	}
	string firstLine;
	std::getline(file, firstLine);
	//get number of dtree nodes
	vector<string> tokens; // Create vector to hold our words	
	boost::split(tokens,firstLine,boost::is_any_of(" \t"),boost::token_compress_on);
	size_t numOfNodes = Convert::string_to_T<size_t>(tokens[1]);
	DTreeNode** nodes = new DTreeNode*[numOfNodes];
	
	std::string line;
	size_t lineNo = 0;
	while (std::getline(file, line)){
		DTreeNode* node = NULL;
		vector<string> tokens; // Create vector to hold our words	
		boost::split(tokens,line,boost::is_any_of(" \t"),boost::token_compress_on);
		if(tokens.at(0).compare("L") == 0){ //a leaf node
			node = new DTreeNode(lineNo,NULL,NULL);
			size_t clauseId = Convert::string_to_T<size_t>(tokens[1]);
			CTerm& term = FM->getTerms().at(clauseId);
			const std::vector<varType>&  lits = term.getLits();
			for( std::vector<varType>::const_iterator it = lits.begin() ; it != lits.end() ; ++it){
				varType v_ = *it;
				varType v = (v_ > 0)  ? v_ : -v_;
				node->vars.push_back(v);
			}			
		}
		else{ //tokens.at(0).compare("I") == 0
			size_t leftIdx = Convert::string_to_T<size_t>(tokens[1]);
			size_t rightIdx = Convert::string_to_T<size_t>(tokens[2]);
			DTreeNode* left = nodes[leftIdx];
			DTreeNode* right = nodes[rightIdx];
			node = new DTreeNode(lineNo,left,right);
			node->vars = left->vars;
			List rightVarsCpy =  right->vars;
			Utils::unionLists(node->vars, rightVarsCpy);
			left->parent = node;
			right->parent=node;
		}
		nodes[lineNo++]=node;
	}
	file.close();	
	DTreeNode* root = nodes[numOfNodes-1];
	delete[] nodes;
	annotateTree(root);
	cout << " dtree stats: " << root->treeStats();
	return root;
}


// AnnotateTree does a treewalk of the decomposition tree and records various
// useful statistics about the tree.  Each node records its acutset (ancestor
// cutset), cutset, context, and cluster.
void DTreeNode::annotateTree(DTreeNode* d_tree){
	// Determine acutset
	if( d_tree->parent != NULL ) {
		List parent_cutset = d_tree->parent->cutset;
		List parent_acutset = d_tree->parent->acutset;
		Utils::unionLists(parent_cutset,parent_acutset);
		d_tree->acutset = parent_cutset;    
	}

   // Determine cutset
	if( d_tree->left != NULL && d_tree->right != NULL ) {
		List left_vars = d_tree->left->vars;
		List right_vars = d_tree->right->vars;
		Utils::intersectLists(left_vars,right_vars,d_tree->cutset);		  
	}
	else{
		d_tree->cutset = d_tree->vars;		
	}
	Utils::deleteIntersection(d_tree->acutset,d_tree->cutset);  
	
	// Determine context
	List vars = d_tree->vars;
	List acutset = d_tree->acutset;
	Utils::intersectLists(vars,acutset,d_tree->context);
    
	 // Determine cluster
	if( d_tree->left == NULL && d_tree->right == NULL ) {
		d_tree->cluster = d_tree->vars;
	}
	else {
		List cutset = d_tree->cutset;
		List context = d_tree->context;
		Utils::unionLists(cutset,context);
		d_tree->cluster = cutset;
	}

	if( d_tree->left != NULL ) {
		annotateTree( d_tree->left);
	}

	if( d_tree->right != NULL ) {
		annotateTree( d_tree->right);
	}

	
	 // Determine elimination order
	if( d_tree->left == NULL && d_tree->right == NULL ) {    
		for( ListIterator li = d_tree->cutset.begin(); li != d_tree->cutset.end(); li++ ) {
			d_tree->elim_order += Convert::T_to_string<varType>(*li);			
			d_tree->elim_order += " ";
		}          
	}
	else {		
		d_tree->elim_order += d_tree->left->elim_order;
		d_tree->elim_order += d_tree->right->elim_order;
		for( ListIterator li = d_tree->cutset.begin(); li != d_tree->cutset.end(); li++ ) {
			d_tree->elim_order += Convert::T_to_string<varType>(*li);	
			d_tree->elim_order += " ";
		} 		
	}

	
	// Erase strings of internal nodes to save on memory, if necessary.
	if( d_tree->left != NULL ) {		
		d_tree->left->elim_order = "";
	}

	if( d_tree->right != NULL ) {		
		d_tree->right->elim_order = "";
	}
	

  // Determine height
	if( d_tree->left == NULL && d_tree->right == NULL ) {
		d_tree->height = 1;
	}
	else {
		if( d_tree->left->height > d_tree->right->height ) {
			d_tree->height = d_tree->left->height + 1;
		}
		else {
			d_tree->height = d_tree->right->height + 1;
		}
	}

	// Determine log width	
	if( d_tree->left == NULL && d_tree->right == NULL ) {
		d_tree->max_cutset = d_tree->cutset.size();
		d_tree->max_context = d_tree->context.size();
		d_tree->max_cluster = d_tree->cluster.size();
		d_tree->max_acutset = d_tree->acutset.size();		
	}
	else {
	d_tree->max_cutset = Utils::umax( d_tree->cutset.size(),
						d_tree->left->max_cutset,
						d_tree->right->max_cutset );
	d_tree->max_context = Utils::umax( d_tree->context.size(),
						d_tree->left->max_context,
						d_tree->right->max_context );
	d_tree->max_cluster = Utils::umax( d_tree->cluster.size(),
						d_tree->left->max_cluster,
						d_tree->right->max_cluster );
	d_tree->max_acutset = Utils::umax( d_tree->acutset.size(),
						d_tree->left->max_acutset,
						d_tree->right->max_acutset );	
	}

}


string DTreeNode::treeStats(){
	std::stringstream ss;
	ss << "Max Cluster=" << max_cluster << ", Max Cutset=" << max_cutset << ", Max Context=" <<  max_context << ", Max Acutset=" <<  max_acutset
		<< ", Dtree Height=" << height << endl;
	return ss.str();
	
}