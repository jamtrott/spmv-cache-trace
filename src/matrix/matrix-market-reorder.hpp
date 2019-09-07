#ifndef MATRIX_MARKET_REORDER_HPP
#define MATRIX_MARKET_REORDER_HPP

#include "matrix-market.hpp"

// produce a re-ordering vector based on the reverse Cuthill-McKee algorithm
std::vector<int> find_new_order_RCM (matrix_market::Matrix const & m);

// produce a re-ordering vector based on "clustering" (via K-way graph partitioning)
std::vector<int> find_new_order_GP (matrix_market::Matrix const & m, int nparts=0);

#endif
