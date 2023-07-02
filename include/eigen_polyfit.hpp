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
std::vector<T> polyfit(const std::vector<T> &yValues, const int degree, const std::vector<T>& weights = std::vector<T>(), bool useJacobi = true)
{
    using namespace Eigen;
    
    bool useWeights = weights.size() > 0 && weights.size() == yValues.size();
    
    int numCoefficients = degree + 1;
    size_t nCount = yValues.size();
    
    MatrixXf X(nCount, numCoefficients);
    MatrixXf Y(nCount, 1);
    
    // fill Y matrix
    for (size_t i = 0; i < nCount; i++)
    {
        if (useWeights)
            Y(i, 0) = yValues[i] * weights[i];
        else
            Y(i, 0) = yValues[i];
    }
    
    // fill X matrix (Vandermonde matrix)
    for (size_t nRow = 0; nRow < nCount; nRow++)
    {
        T nVal = 1.0f;
        for (int nCol = 0; nCol < numCoefficients; nCol++)
        {
            if (useWeights)
                X(nRow, nCol) = nVal * weights[nRow];
            else
                X(nRow, nCol) = nVal;
            nVal *= nRow;
        }
    }
    
    VectorXf coefficients;
    if (useJacobi)
        coefficients = X.jacobiSvd(ComputeThinU | ComputeThinV).solve(Y);
    else
        coefficients = X.colPivHouseholderQr().solve(Y);
    
    return std::vector<T>(coefficients.data(), coefficients.data() + numCoefficients);
}
