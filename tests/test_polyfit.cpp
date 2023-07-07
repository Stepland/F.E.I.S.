#include "Eigen/Dense"
#include <iostream>

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

int main(void) {
    // std::vector<float> x = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<float> y = {0.0, 0.8, 0.9, 0.1, -0.8, -1.0};
    auto coeffs = polyfit(y, 3);
    for (const auto& coeff : coeffs) {
        std::cout << coeff << "\n";
    }
}