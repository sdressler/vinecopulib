/*
* The MIT License (MIT)
*
* Copyright © 2017 Thibault Vatter and Thomas Nagler
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "rvine_matrix.hpp"


RVineMatrix::RVineMatrix(const MatXi& matrix)
{
    d_ = matrix.rows();
    // TODO: sanity checks for input matrix
    matrix_ = matrix;
}

MatXi RVineMatrix::get_matrix()
{
    return matrix_;
}

VecXi RVineMatrix::get_order()
{
    return matrix_.colwise().reverse().diagonal().reverse();
}

//! Construct a D-vine matrix 
//! 
//! A D-vine is a vine where each tree is a path.
//! 
//! @param order order of the variables
//! 
//! @return An Eigen::MatrixXi describing the D-vine structure.
MatXi RVineMatrix::construct_d_vine_matrix(const VecXi& order)
{
    int d = order.size();
    MatXi vine_matrix = MatXi::Constant(d, d, 0);
    
    for (int i = 0; i < d; ++i) {
        vine_matrix(d - 1 - i, i) = order(d - 1 - i);  // diagonal
    }
    
    for (int i = 1; i < d; ++i) {
        for (int j = 0; j < i; ++j) {
            vine_matrix(d - 1 - i, j) = order(i - j - 1);  // below diagonal
        }
    }

    return vine_matrix;
}


//! Reorder R-vine matrix to natural order 
//! 
//! Natural order means that the diagonal has entries (d, ..., 1). We convert
//! to natural order by relabeling the variables. Most algorithms for estimation
//! and evaluation assume that the matrix is in natural order.
//! 
//! @parameter matrix initial R-vine matrix.
//! 
//! @return An Eigen::MatrixXi containing the matrix in natural order.
MatXi RVineMatrix::in_natural_order() 
{
    // create vector of new variable labels: d, ..., 1
    std::vector<int> ivec = stl_tools::seq_int(1, d_);
    stl_tools::reverse(ivec);
    Eigen::Map<VecXi> new_labels(&ivec[0], d_);     // convert to VecXi
    
    return relabel_elements(matrix_, new_labels);
}

//! Get maximum matrix
//! 
//! The maximum matrix is derived from an R-vine matrix by iteratively computing
//! the (elementwise) maximum of a row and the row below (starting from the 
//! bottom). It is used in estimation and evaluation algorithms to find the right
//! pseudo observations for an edge.
//! 
//! @parameter no_matrix initial R-vine matrix, assumed to be in natural order.
//! 
//! @return An Eigen::MatrixXi containing the maximum matrix
MatXi RVineMatrix::get_max_matrix() 
{
    MatXi max_matrix = this->in_natural_order();
    for (int i = 0; i < d_ - 1; ++i) {
        for (int j = 0 ; j < d_ - i - 1; ++j) {
            max_matrix(i + 1, j) = max_matrix.block(i, j, 2, 1).maxCoeff();
        }
    }
    return max_matrix;
}

//! Obtain matrix indicating which h-functions are needed
//! 
//! It is usually not necessary to apply both h-functions for each pair-copula.
//! 
//! @parameter no_matrix initial R-vine matrix (assumed to be in natural order).
//! 
//! @return An Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> indicating
//! whether hfunc1/2 is needed for a given pair copula.
//! 
//! @{
MatXb RVineMatrix::get_needed_hfunc1() 
{
    MatXb needed_hfunc1 = MatXb::Constant(d_, d_, false);
    
    MatXi no_matrix = this->in_natural_order();
    MatXi max_matrix = this->get_max_matrix();
    for (int i = 1; i < d_ - 1; ++i) {
        int j = d_ - i;
        MatXb isnt_mat_j = (no_matrix.block(0, 0, j, i).array() != j);
        MatXb is_max_j = (max_matrix.block(0, 0, j, i).array() == j);
        MatXb is_different = (isnt_mat_j.array() && is_max_j.array());
        needed_hfunc1.block(0, i, j, 1) = is_different.rowwise().any();
    }    
    return needed_hfunc1;
}

MatXb RVineMatrix::get_needed_hfunc2() 
{
    MatXb needed_hfunc2 = MatXb::Constant(d_, d_, false);
    needed_hfunc2.block(0, 0, d_ - 1, 1) = MatXb::Constant(d_ - 1, 1, true);
    MatXi no_matrix = this->in_natural_order();
    MatXi max_matrix = this->get_max_matrix();
    for (int i = 1; i < d_ - 1; ++i) {
        int j = d_ - i;
        // fill column i with true above the diagonal
        needed_hfunc2.block(0, i, d_ - i, 1) = MatXb::Constant(d_ - i, 1, true);
        // for diagonal, check whether matrix and maximum matrix coincide
        MatXb is_mat_j = (no_matrix.block(j - 1, 0, 1, i).array() == j);
        MatXb is_max_j = (max_matrix.block(j - 1, 0, 1, i).array() == j);
        needed_hfunc2(j - 1, i) = (is_mat_j.array() && is_max_j.array()).any();
    }
    
    return needed_hfunc2;
}
//! @}

// translates matrix_entry from old to new labels
int relabel_one(const int& x, const VecXi& old_labels, const VecXi& new_labels)
{
    for (unsigned int i = 0; i < old_labels.size(); ++i) {
        if (x == old_labels[i])
            return new_labels[i];
    }
    return 0;
}

// relabels all elements of the matrix (upper triangle assumed to be 0)
MatXi relabel_elements(const MatXi& matrix, const VecXi& new_labels)
{
    int d = matrix.rows();
    VecXi old_labels = matrix.colwise().reverse().diagonal();
    MatXi new_matrix = MatXi::Zero(d, d);
    for (int i = 0; i < d; ++i) {
        for (int j = 0; j < d - i; ++j) {
            new_matrix(i, j) = relabel_one(matrix(i, j), old_labels, new_labels);
        }
    }
    
    return new_matrix;
}
