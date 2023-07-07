//  Adapted from https://github.com/patLoeber/Polyfit
//
//  PolyfitEigen.hpp
//  Polyfit
//
//  Created by Patrick Löber on 23.11.18.
//  Copyright © 2018 Patrick Loeber. All rights reserved.
//
//  Use the Eigen library for fitting: http://eigen.tuxfamily.org
//  See https://eigen.tuxfamily.org/dox/group__TutorialLinearAlgebra.html for different methods

#include "Eigen/Dense"

template<typename T>
std::vector<T> polyfit(const std::vector<T> &yValues, const int degree) {
    using namespace Eigen;
    
    int coeffs = degree + 1;
    size_t samples = yValues.size();
    
    MatrixXf A(samples, coeffs);
    MatrixXf b(samples, 1);
    
    // fill b matrix
    // fill A matrix (Vandermonde matrix)
    for (size_t row = 0; row < samples; row++) {
        for (int col = 0; col < coeffs; col++) {
            A(row, col) = std::pow(row, col);
        }
        b(row) = yValues[row];
    }
    
    // Solve Ax = b
    VectorXf coefficients;
    coefficients = A.householderQr().solve(b);
    return std::vector<T>(coefficients.data(), coefficients.data() + coeffs);
}
