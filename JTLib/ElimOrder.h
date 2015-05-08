// elimorder.h
// Header file for the elimination order object (ElimOrder)

#ifndef ELIMORDER_H
#define ELIMORDER_H

#include <vector>
#include <string>

using namespace std;
typedef vector<int> IntVector;

class ElimOrder {

 public:
  ElimOrder();

  void appendElement( int e );
  // append an element to the end of the elimination order

  int element( int index );
  // return the element referenced by index

  int size();
  // return the number of elements in the elimination order

  string toString();
  // return the ElimOrder as a string
  
 private:
  IntVector order;
  string Int2String( int a );


};

#endif
