#include "matrix-market-reorder.hpp"

#include <queue>
#include <iostream>
#include <algorithm>

#ifdef USE_METIS
#include "metis.h"
#endif

using namespace std;


void generate_degree_and_adjacency (matrix_market::Matrix const & m,
				    vector<int> & degrees,
				    vector<vector<int> > & adjacency)
{
  if (m.format() != matrix_market::Format::coordinate)
    throw runtime_error("Expected matrix in coordinate format");

  cout<<"Number of rows="<<m.size().rows<<" columns="<<m.size().columns
      <<" entries="<<m.size().num_entries<<"\n";

  if (m.size().rows!=m.size().columns)
    throw runtime_error("Expected a square matrix");

  if (m.field() != matrix_market::Field::real)
    throw runtime_error("Expected matrix with real values");

  vector<matrix_market::CoordinateEntryReal> entries = m.coordinate_entries_real();
  degrees.resize(m.size().rows); degrees.assign(m.size().rows, 0);

  int bandwidth=0;
  for (int e=0; e<m.size().num_entries; e++)
    if (entries[e].i != entries[e].j) {
      /*
      if (entries[e].i<=0 || entries[e].i>m.size().rows || entries[e].j<=0 || entries[e].j>m.size().rows)
	cout<<"i="<<entries[e].i<<" j="<<entries[e].j<<" rows="<<m.size().rows<<"\n";
      */
      degrees[entries[e].i-1]++;
      bandwidth = max(bandwidth,abs(entries[e].i-entries[e].j));
    }
  cout<<"Bandwidth of the matrix is "<<bandwidth<<"\n";

  adjacency.resize(m.size().rows);
  for (int i=0; i<m.size().rows; i++) {
    adjacency[i].resize(degrees[i]);
    degrees[i] = 0;
  }
  
  for (int e=0; e<m.size().num_entries; e++)
    if (entries[e].i != entries[e].j) {
      adjacency[entries[e].i-1][degrees[entries[e].i-1]] = entries[e].j-1;
      degrees[entries[e].i-1]++;
    }
}


// produce a re-ordreing (permutation) vector using the reverse Cuthill-McKee algorithm
vector<int> find_new_order_RCM (matrix_market::Matrix const & m)
{
  vector<int> degrees;
  vector<vector<int> > adjacency;
  generate_degree_and_adjacency (m, degrees, adjacency);
  
  vector<int> R;
  vector<int> notTaken; notTaken.assign(degrees.size(),1);
  vector<int> notVisited; notVisited.assign(degrees.size(),1);
  int num_not_taken = degrees.size();

  cout<<"Running the reverse Cuthill-McKee algorithm to find another permutation\n";
  
  while (num_not_taken) { 
  
	  int minNodeIndex = -1, min_degree = degrees.size();
  
            for (int i = 0; i < degrees.size(); i++) 
	      if (notTaken[i] && degrees[i]<min_degree) {
		minNodeIndex = i;
	        min_degree = degrees[i];
	      }
	    //cout<<"minNodeIndex="<<minNodeIndex<<" degree="<<degrees[minNodeIndex]<<" num notTaken="<<num_not_taken<<"\n";
 
	    R.push_back(minNodeIndex);
	    notTaken[minNodeIndex] = 0;
	    notVisited[minNodeIndex] = 0;
	    num_not_taken--;
	    
	    vector<int> toSort;
	    for (int j = 0; j < degrees[minNodeIndex]; j++) {
	      int k = adjacency[minNodeIndex][j];
	      if (notVisited[k]) {
		toSort.push_back(k);
		notVisited[k] = 0;
	      }
	    }
	    
	    if (toSort.size()>1)
	      sort(toSort.begin(), toSort.end(),
		   [&degrees](int i1, int i2) { return degrees[i1]<degrees[i2]; });

	    queue<int> Q; 
	    for (int j = 0; j < toSort.size(); j++) 
	      Q.push(toSort[j]);

	    
	    while (!Q.empty()) {
	      int i = Q.front();

	      if (notTaken[i]) {
		R.push_back(i);
		notTaken[i] = 0;
		notVisited[i] = 0;
		num_not_taken--;

		vector<int> toSort_;
		for (int j = 0; j < degrees[i]; j++) {
		  int k = adjacency[i][j];
		  if (notVisited[k]) {
		    toSort_.push_back(k);
		    notVisited[k] = 0;
		  }
		}

		if (toSort_.size()>1)
		  sort(toSort_.begin(), toSort_.end(),
		       [&degrees](int i1, int i2) { return degrees[i1]<degrees[i2]; });

		for (int j = 0; j < toSort_.size(); j++)
		  Q.push(toSort_[j]);
		}

	      Q.pop();
	    }
  }

  // reverse the order
  int n = R.size(); 
  
  if (n % 2 == 0) 
    n -= 1; 
  
  n = n / 2; 
  
  for (int i = 0; i <= n; i++) { 
    int j = R[R.size() - 1 - i]; 
    R[R.size() - 1 - i] = R[i]; 
    R[i] = j; 
  }

  vector<int> new_order;
  new_order.assign(m.size().rows, -1);
  for (int i = 0; i<R.size(); i++) {
    if (R[i]<0 || R[i]>=m.size().rows)
      cout<<"Impossible values of R["<<i<<"] value: "<<R[i]<<"\n";
    new_order[R[i]] = i;
  }

  for (int i = 0; i<new_order.size(); i++)
    if (new_order[i]<0 || new_order[i]>=m.size().rows)
      cout<<"Impossible value of new_order["<<i<<"] value: "<<new_order[i]<<"\n";

  vector<matrix_market::CoordinateEntryReal> entries = m.coordinate_entries_real();
  int bandwidth=0;
  for (int e=0; e<m.size().num_entries; e++)
    bandwidth = max(bandwidth,abs(new_order[entries[e].i-1]-new_order[entries[e].j-1]));
  cout<<"New bandwidth of the matrix is "<<bandwidth<<"\n";

  return new_order;
}

#ifndef USE_METIS
vector<int> find_new_order_GP (matrix_market::Matrix const & m, int nparts)
{
  cout<<"Warning: No reordering is done. You should compile with 'USE_METIS' defined\n";
  vector<int> same_order(m.size().rows);
  for (int i=0; i<m.size().rows; i++)
    same_order[i] = i;
  return same_order;
}
#else
// produce a re-ordering vector based on "clustering" (via K-way graph partitioning)
vector<int> find_new_order_GP (matrix_market::Matrix const & m, int nparts)
{
  if (m.format() != matrix_market::Format::coordinate)
    throw runtime_error("Expected matrix in coordinate format");

  cout<<"Number of rows="<<m.size().rows<<" columns="<<m.size().columns
      <<" entries="<<m.size().num_entries<<"\n";

  if (m.size().rows!=m.size().columns)
    throw runtime_error("Expected a square matrix");

  if (m.field() != matrix_market::Field::real)
    throw runtime_error("Expected matrix with real values");

  idx_t nvtxs, ncon, *xadj, *adjncy, edgecut, *part;
  real_t ubvec = 1.05;  // load imbalance threshold value for partitioning

  nvtxs = m.size().rows;
  ncon = 1;
  xadj = (idx_t*)malloc((nvtxs+1)*sizeof(idx_t));

  int i,k,e,num_edges=0;
  for (i=0; i<=m.size().rows; i++)
    xadj[i] = 0;
  
  vector<matrix_market::CoordinateEntryReal> entries = m.coordinate_entries_real();
  
  for (e=0; e<m.size().num_entries; e++)
    if (entries[e].i != entries[e].j) {
      xadj[entries[e].i]++; num_edges++;   // values of entries start from 1 (not 0)
    }
  cout<<"total num edges="<<num_edges<<"\n";

  int *offset = (int*)malloc(m.size().rows*sizeof(int));
  for (i=1; i<=m.size().rows; i++) {
    offset[i-1] = xadj[i-1];
    xadj[i] += xadj[i-1];
  }
  cout<<"xadj[rows]="<<xadj[m.size().rows]<<"\n";

  adjncy = (idx_t*)malloc(num_edges*sizeof(idx_t));
  for (e=0; e<m.size().num_entries; e++)
    if (entries[e].i != entries[e].j) {
      k = entries[e].i-1;
      adjncy[offset[k]] = entries[e].j-1;
      offset[k]++;
    }

  for (i=1; i<=m.size().rows; i++)
    if (xadj[i]!=offset[i-1])
      cout<<"xadj["<<i<<"]="<<xadj[i]<<" offset["<<i-1<<"]="<<offset[i-1]<<"\n";

  if (nparts<=1)
    nparts = 16;
  cout<<"Running METIS_PartGraphKway with nparts="<<nparts<<"\n";
  
  part = (idx_t*)malloc(nvtxs*sizeof(idx_t));
  METIS_PartGraphKway(&nvtxs, &ncon, xadj, adjncy, NULL, NULL, NULL,
		      &nparts, NULL, &ubvec, NULL, &edgecut, part);

  for (i=1; i<=nparts; i++)
    offset[i] = 0;
  for (i=0; i<nvtxs; i++)
    offset[part[i]+1]++;
  offset[0] = 0;
  for (i=1; i<=nparts; i++)
    offset[i] += offset[i-1];

  vector<int> permutation(nvtxs);
  for (i=0; i<nvtxs; i++) {
    permutation[offset[part[i]]] = i;
    offset[part[i]]++;
  }
  
  vector<int> new_order;
  new_order.assign(nvtxs, -1);
  for (int i = 0; i<nvtxs; i++) {
    if (permutation[i]<0 || permutation[i]>=nvtxs)
      cout<<"Impossible values of permutation["<<i<<"] value: "<<permutation[i]<<"\n";
    new_order[permutation[i]] = i;
  }

  for (int i = 0; i<new_order.size(); i++)
    if (new_order[i]<0 || new_order[i]>=m.size().rows)
      cout<<"Impossible value of new_order["<<i<<"] value: "<<new_order[i]<<"\n";
	/*
  int bandwidth=0;
  for (int e=0; e<m.size().num_entries; e++)
    bandwidth = max(bandwidth,abs(new_order[entries[e].i-1]-new_order[entries[e].j-1]));
  cout<<"New bandwidth of the matrix is "<<bandwidth<<"\n";
	*/

  free(xadj); free(offset); free(adjncy); free(part);
	
  return new_order;
}
#endif

//(permute a matrix-market matrix following a new order)
/*
matrix_market::Matrix permute_matrix (matrix_market::Matrix const & m,
				      vector<int> const & permutation)
{
  if (m.format() != matrix_market::Format::coordinate)
    throw runtime_error("Expected matrix in coordinate format");

  vector<matrix_market::CoordinateEntryReal> entries = m.coordinate_entries_real();
  for (int e=0; e<m.size().num_entries; e++) {
    entries[e].i = permutation[entries[e].i-1]+1;
    entries[e].j = permutation[entries[e].j-1]+1;
  }

  return matrix_market::Matrix(m.header(), m.comments(), m.size(), entries);
}
*/
