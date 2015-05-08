#ifndef UTILS_H
#define UTILS_H

#include <boost/foreach.hpp>
#include "boost/range/algorithm/set_algorithm.hpp"
#include "Definitions.h"
#include <list>
#include <sstream>
#include <time.h>
#include <math.h>  
using namespace boost;
using namespace std;

#define EPSILON 0.0000001
class Utils{
public:
	static long timeSpentSettingAssignment;
	static long timeSpentClearingAssignment;
	static long timeSpentIncrementing;
	static long timeSpentTCFDtor;
	static long treeCPTPrunes;

	//subtracts vars from bitset
	static void subt(DBS& BS, const varSet& vars){
		varSet::const_iterator end = vars.end();
		for(size_t v = BS.find_first() ; v != DBS::npos ; v = BS.find_next(v)){
			if(vars.find((int) v) !=end){
				BS.set(v,false);
			}
		}		
	}
	
	template<class T>
	//return set1-set2 (i.e; all members of set1 that are not in set2)
	static void subt(const boost::unordered_set<T>&  set1, const boost::unordered_set<T>& set2, boost::unordered_set<T>& out_retVal){		
		typename boost::unordered_set<T>::const_iterator end = set1.cend() ;
		for(typename boost::unordered_set<T>::const_iterator cit = set1.cbegin() ; cit != end ; ++cit){
			if(!contains(set2,*cit)){
				out_retVal.insert(*cit);
			}
		}		
	}

	template<class T>
	//return set1-set2 (i.e; all members of set1 that are not in set2)
	static void subt(vector<T>&  sortedVec1, const vector<T>& sortedVec2){
		if(sortedVec1.empty() || sortedVec2.empty()) return;

		vector<T> tempOut(sortedVec1.size());
		set_difference(sortedVec1.begin(), sortedVec1.end(), sortedVec2.begin(), sortedVec2.end(), std::back_inserter(tempOut));
		sortedVec1.assign(tempOut.cbegin(), tempOut.cend());		
	}

	template<class T>
	//return set1-set2 (i.e; all members of set1 that are not in set2)
	static void subt(boost::unordered_set<T>&  set1, const boost::unordered_set<T>& set2){
		typename boost::unordered_set<T>::const_iterator set2End = set2.cend();
		for(typename boost::unordered_set<T>::const_iterator cit = set2.cbegin() ; cit != set2End ; ++cit){
			typename boost::unordered_set<T>::iterator set1It = set1.find(*cit);
			if(set1It != set1.end()){
				set1.quick_erase(set1It);
			}			
		}		
	}

	template<class T>
	static void intersectByLists(const boost::unordered_set<T>& set1, const boost::unordered_set<T>& set2, boost::unordered_set<T>& out){
		if(set1.empty() || set2.empty()) return;
		std::list<T> L1,L2,L3;
		toList(set1,L1);
		toList(set2,L2);
		Utils::intersectLists(L1,L2,L3);
		typename std::list<T>::const_iterator toIterateEnd = L3.cend();
		for(typename std::list<T>::const_iterator cit = L3.cbegin() ; cit != toIterateEnd ; ++cit){
			out.insert(*cit);
		}

	}

	template<class T>
	static void toList(const boost::unordered_set<T>& set1, std::list<T>& L){
		typename boost::unordered_set<T>::const_iterator toIterateEnd = set1.cend();
		for(typename boost::unordered_set<T>::const_iterator cit = set1.cbegin() ; cit != toIterateEnd ; ++cit){
			L.push_back(*cit);
		}
	}

	template<class T>
	static T* toArray(const boost::unordered_set<T>& set1){
		T* arr = new T[set1.size()];		
		typename boost::unordered_set<T>::const_iterator toIterateEnd = set1.cend();
		size_t i =0;
		for(typename boost::unordered_set<T>::const_iterator cit = set1.cbegin() ; cit != toIterateEnd ; ++cit){
			arr[i++]=*cit;			
		}	
		return arr;
	}

	static void toDBS(const boost::unordered_set<varType>& set1, DBS& retVal ){
		boost::unordered_set<varType>::const_iterator toIterateEnd = set1.cend();
		for(boost::unordered_set<varType>::const_iterator cit = set1.cbegin() ; cit != toIterateEnd ; ++cit){
			retVal.set(*cit,true);			
		}	
	}

	template<class T>
	static void intersect(const boost::unordered_set<T>& set1, const boost::unordered_set<T>& set2, boost::unordered_set<T>& out){
		if(set1.empty() || set2.empty()) return;

		const boost::unordered_set<T>& toIterate = (set1.size() <= set2.size()) ? set1 : set2;
		const boost::unordered_set<T>& other = (set1.size() <= set2.size()) ? set2 : set1;
		
		typename boost::unordered_set<T>::const_iterator toIterateEnd = toIterate.cend();
		for(typename boost::unordered_set<T>::const_iterator cit = toIterate.cbegin() ; cit != toIterateEnd ; ++cit){
			if(other.find(*cit) != other.end()){
				out.insert(*cit);
			}
		}
		return;
	}

	template<class T>
	static void intersect(const boost::unordered_set<T>& set1, boost::unordered_set<T>& out){
		if(set1.empty()){
			out.clear();
			return;
		}
		typename boost::unordered_set<T>::iterator cit = out.begin();
		while(cit != out.end()){
			if(set1.find(*cit) == set1.end()){
				cit = out.erase(cit);
			}
			else{
				++cit;
			}

		}		
	}
	
	/*
	returns true if DBS1 contains DBS2
	*/
	static bool contains(const DBS& DBS1, const DBS& DBS2){
		if(DBS2.count() > DBS1.count()){
			return false;
		}
		DBS temp(DBS2);
		temp.operator&=(DBS1); //temp contains the intersection DBS1 ^ DBS2 
		if(temp==DBS2){ // If DBS2==(DBS1 ^ DBS2) then DBS2 \subseteq DBS1
			return true;
		}
		return false;
	}

	static void fromSetToDBS(DBS& BS, const varSet& vars){
		BS.reset();
		varSet::const_iterator end = vars.end();
		for(varSet::const_iterator cit = vars.cbegin() ; cit != end ; ++cit){
			varType v = *cit;
			size_t vidx = (size_t)((v > 0) ? v : -v);
			BS.set(vidx,true);
		}
	}
/*	template<class T>
	static boost::unordered_set<T>* intersect(const boost::unordered_set<T>& set1, const boost::unordered_set<T>& set2){
		boost::unordered_set<T>* retVal = new boost::unordered_set<T>();
		intersect(set1,set2,*retVal);
		return retVal;
	}*/

	static string printDBS(const DBS& BS, string delim=", ", string end="", bool newline = true){
		std::stringstream ss;
		bool first = true;
		for(size_t i = BS.find_first() ; i != boost::dynamic_bitset<>::npos ; i = BS.find_next(i)){
			if(!first){
				ss << delim;
			}
			else{
				first = false;
			}
			ss << i;
		}
		ss << " " << end;
		if(newline)
			ss << endl;
		return ss.str();	
	}
	
	template<class T>
	static string printList(const list<T>& list){
		std::stringstream ss;
		BOOST_FOREACH(T t, list){
			ss << t << ",";
		}
		ss << endl;
		return ss.str();
	}
	
	template<class T>
	static string printList(const list<boost::unordered_set<T> >& list){
		std::stringstream ss;
		int i=1;
		BOOST_FOREACH(boost::unordered_set<T> set, list){
			ss << "set " << i++ << ":" << Utils::printSet(set);
		}
		ss << endl;
		return ss.str();
	}

	template<class T>
	static string printSet(const boost::unordered_set<T>& set){
		std::stringstream ss;
		BOOST_FOREACH(T t, set){
			ss << t << ",";
		}
		ss << endl;
		return ss.str();
	}

	template<class T>
	static string printMap(const unordered_map<T,unordered_set<T> >& map){
		std::stringstream ss;
		for(typename unordered_map<T,unordered_set<T> >::const_iterator cit = map.cbegin() ; cit != map.cend() ; ++cit){
			ss << cit->first << ":";
			ss << printSet(cit->second);
		}
		ss << endl;
		return ss.str();
	}

	template<class T>
	static string printMap(const unordered_map<T,T>& map){
		std::stringstream ss;
		for(typename unordered_map<T,T>::const_iterator cit = map.cbegin() ; cit != map.cend() ; ++cit){
			ss << cit->first << ":" << cit->second << endl;			
		}
		ss << endl;
		return ss.str();
	}

	template<class T>
	//returns true if set contins v
	static bool contains(const boost::unordered_set<T>& set, const T& v){
		typename boost::unordered_set<T>::const_iterator cit = set.find(v);
		if(cit== set.cend()) return false;		
		return true;
	}
	
		template<class T>
	//returns true if set1 contains set2
	static bool contains(const boost::unordered_set<T>& set1, const boost::unordered_set<T>& set2){
		if(set2.size() > set1.size()) return false;
		typename boost::unordered_set<T>::const_iterator set2End = set2.cend();
		for(typename boost::unordered_set<T>::const_iterator cit = set2.cbegin() ; cit != set2End ; ++cit){
			if(!contains(set1,*cit))
				return false;
		}
		return true;
	}
	//returns true if set1 contains set2 using absolute values
	static bool containsAbs(const varSet& set1, const varSet& set2){
		if(set2.size() > set1.size()) return false;
		varSet::const_iterator set2End = set2.cend();
		for(varSet::const_iterator cit = set2.cbegin() ; cit != set2End ; ++cit){
			varType v = *cit;
			if(!contains(set1,v) && !contains(set1,0-v))
				return false;
		}
		return true;
	}

	static void convertToVarSetAbs(const vector<varType>& vec, varSet& retVal){
		retVal.clear();
		for(vector<varType>::const_iterator varIt = vec.begin() ; varIt != vec.end() ; ++varIt){
			varType lit = *varIt;
			varType varAbs = (lit > 0)  ? lit : 0-lit;
			retVal.insert(varAbs);
		}
	}

	static void enhanceWithSignedVars(varSet& vars){
		varSet negatedVars;
		varSet::const_iterator varsEnd = vars.cend();
		for(varSet::const_iterator varsIt = vars.begin() ; varsIt != varsEnd ; ++varsIt){
			varType negV = (0-*varsIt);
			negatedVars.insert(negV);
		}
		vars.insert(negatedVars.begin(), negatedVars.end());
	}

	static void removeNegativeVars(list<varType>& listVars){
		list<varType>::iterator it = listVars.begin();
		while(it != listVars.end()){
			if(*it < 0){
				it = listVars.erase(it);
			}
			else{
				++it;
			}
		}
	}

	template<class T>
	//returns true if set1 properly contains set2
	static bool properContains(const boost::unordered_set<T>& set1, const boost::unordered_set<T>& set2){
		if(set1.size() <= set2.size()) //--> set1 > set2
			return false;
		return Utils::contains(set1,set2);		
	}
	
	static const char* getMsg(string str){
		std::stringstream ss;
		ss << str << endl;
		return ss.str().c_str();
	}

	template<class T>
	static void deleteFromVector(std::vector<T>& vec, T t){
		size_t tInd;
		for(size_t tInd=0 ;  tInd < vec.size() ; tInd++){
			if(vec[tInd]==t){
				break;
			}
		}
		if(tInd < vec.size()){
			vec.erase(vec.begin()+tInd);
		}
	}

	static size_t getBitSetSize(const DBS& BS){
		return (sizeof(DBS) + sizeof(DBS::block_type)*BS.num_blocks());
	}


	struct TimeOutException : public std::exception
	{
	   std::string s;
	   TimeOutException(string ss) : s(ss) {}
	   ~TimeOutException() throw () {} // Updated
	   const char* what() const throw() { return s.c_str(); }
	};

	static double startTime;
	static double timeLimitSec;
	static bool timedOut;
	static void setStartTime(){
		startTime = get_cpu_time();
	}

	static void setTimeLimit(double timeLimitSec_){
		timeLimitSec = timeLimitSec_;
	}

	static double get_cpu_time()
	{
		return ((double)clock()/(double)(CLOCKS_PER_SEC));
	}

	static bool time_out(){
		timedOut = ((get_cpu_time() - startTime) > timeLimitSec);
		return timedOut;
	}

	//unites both lists into L1
	template<class T>
	static void unionLists(std::list<T>& L1, std::list<T>& L2){
		L1.sort();
		L2.sort();
		L1.merge(L2);
		L1.unique();			
	}

	// intersection will be in L1
	template<class T>
	static void intersectLists(std::list<T>& L1, std::list<T>& L2, std::list<T>& retVal){
		L1.sort();
		L2.sort();		
		while( !L1.empty() && !L2.empty() ) {
			if( L1.front() < L2.front() ) {
				L1.pop_front();
			}
			else if( L1.front() > L2.front() ) {
				L2.pop_front();
			}
			else{
				retVal.push_back(L1.front());
				L1.pop_front();
				L2.pop_front();
			}
		}
	}

	template<class T>
	static void intersectVecsNoSort(const std::vector<T>& L1, const std::vector<T>& L2,  boost::unordered_set<T>& retVal){
		const std::vector<T>& longVec = (L1.size() >= L2.size() ? L1 : L2);
		const std::vector<T>& shortVec= (L1.size() >= L2.size() ? L2 : L1);
		
		for(size_t i = 0 ; i < shortVec.size() ; ++i){
			T val = shortVec[i];
			if(std::binary_search(longVec.begin(), longVec.end(),val)){
				retVal.insert(val);
			}
		}	
	}

	template<class T>
	static void intersectVecsBinSrch(const std::vector<T>& longVec, const std::vector<T>& shortVec,  boost::unordered_set<T>& retVal){
		for(size_t i = 0 ; i < shortVec.size() ; ++i){
			T val = shortVec[i];
			if(std::binary_search(longVec.begin(), longVec.end(),val)){
				retVal.insert(val);
			}
		}	
	}

	template<class T>
	static void intersectVecsLinSrch(const std::vector<T>& L1, const std::vector<T>& L2,  boost::unordered_set<T>& retVal){
		size_t i1 = 0;
		size_t i2 = 0;
		while(i1 < L1.size() && i2 < L2.size()){
			if(L1[i1] < L2[i2]){
				i1++;
			}
			else if(L2[i2] < L1[i1]){
				i2++;
			}
			else{
				retVal.insert(L1[i1]);
				i1++;
				i2++;
			}
		}
	}

	template<class T>
	static void intersectVecsNoSort(std::vector<T>& L1, std::vector<T>& L2,  boost::unordered_set<T>& retVal){
		const std::vector<T>& longVec = (L1.size() >= L2.size() ? L1 : L2);
		const std::vector<T>& shortVec= (L1.size() >= L2.size() ? L2 : L1);
		double longSize = (double)longVec.size();
		double shortSize = (double)shortVec.size();		
		double binSrchCost=shortSize*(log(longSize)/log(2.0));
		double linSrchCost=longSize+shortSize;
		if(linSrchCost <= binSrchCost){
			intersectVecsLinSrch(longVec,shortVec,retVal);
		}
		else{
			intersectVecsBinSrch(longVec,shortVec,retVal);
		}		
	}

	template<class T>
	static void intersectVecs(std::vector<T>& L1, std::vector<T>& L2,  boost::unordered_set<T>& retVal){
		std::sort(L1.begin(),L1.end());
		std::sort(L2.begin(),L2.end());
		intersectVecsNoSort(L1,L2,retVal);
	}
	
	template<class T>
	static void deleteIntersection(const std::list<T>& toDelete, std::list<T>& L){
		for(typename std::list<T>::const_iterator li = toDelete.begin(); li != toDelete.end(); li++ ) {
			L.remove( *li );
		}  
	}

	template<class T>
	static T umax(T a, T b, T c){
		if( a > b && a > c) {
			return a;
		}
		else if( b > c ) {
			return b;
		}
		else {
			return c;
		}
	}

	static bool approximatelyEqual(probType a, probType b, probType epsilon=EPSILON)
	{
		return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}

	static bool essentiallyEqual(probType a, probType b, probType epsilon=EPSILON)
	{
		return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
	}
};


#endif
