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
    
    MatrixXf X(samples, coeffs);
    MatrixXf Y(samples, 1);
    
    // fill Y matrix
    for (size_t i = 0; i < samples; i++) {
        Y(i, 0) = yValues[i];
    }
    
    // fill X matrix (Vandermonde matrix)
    for (size_t row = 0; row < samples; row++) {
        for (int col = 0; col < coeffs; col++) {
            X(row, col) = std::pow(row, col);
        }
    }
    
    VectorXf coefficients;
    coefficients = X.jacobiSvd(ComputeThinU | ComputeThinV).solve(Y);
    return std::vector<T>(coefficients.data(), coefficients.data() + coeffs);
}
