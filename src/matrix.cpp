#include "matrix.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

bool solve_square_system(Matrix A, Matrix b, Matrix& x) {
    int n = A.rows;
    Matrix aug(n, n + 1);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            aug(i, j) = A(i, j);
        }
        aug(i, n) = b(i, 0);
    }

    // Gaussian elimination with partial pivoting
    for (int i = 0; i < n; ++i) {
        int max_row = i;
        double max_val = std::abs(aug(i, i));
        for (int r = i + 1; r < n; ++r) {
            if (std::abs(aug(r, i)) > max_val) {
                max_val = std::abs(aug(r, i));
                max_row = r;
            }
        }

        if (max_val < 1e-12) {
            return false; // Singular matrix
        }

        if (max_row != i) {
            for (int c = i; c <= n; ++c) {
                std::swap(aug(i, c), aug(max_row, c));
            }
        }

        double pivot = aug(i, i);
        for (int c = i; c <= n; ++c) {
            aug(i, c) /= pivot;
        }

        for (int r = 0; r < n; ++r) {
            if (r != i) {
                double factor = aug(r, i);
                for (int c = i; c <= n; ++c) {
                    aug(r, c) -= factor * aug(i, c);
                }
            }
        }
    }

    x = Matrix(n, 1);
    for (int i = 0; i < n; ++i) {
        x(i, 0) = aug(i, n);
    }
    return true;
}

bool Matrix::solve(const Matrix& b, Matrix& x) const {
    if (rows == cols) {
        return solve_square_system(*this, b, x);
    } else if (rows > cols) {
        Matrix At = this->transpose();
        Matrix AtA = At * (*this);
        Matrix Atb = At * b;
        return solve_square_system(AtA, Atb, x);
    }
    return false;
}

Matrix Matrix::inv3x3() const {
    if (rows != 3 || cols != 3) {
        throw std::invalid_argument("inv3x3 requires a 3x3 matrix");
    }

    const Matrix& m = *this;
    double det = m(0,0)*(m(1,1)*m(2,2) - m(1,2)*m(2,1)) -
                 m(0,1)*(m(1,0)*m(2,2) - m(1,2)*m(2,0)) +
                 m(0,2)*(m(1,0)*m(2,1) - m(1,1)*m(2,0));

    if (std::abs(det) < 1e-12) {
        throw std::runtime_error("Matrix is singular, cannot invert 3x3");
    }

    double invdet = 1.0 / det;
    Matrix inv(3, 3);

    inv(0,0) = (m(1,1)*m(2,2) - m(1,2)*m(2,1)) * invdet;
    inv(0,1) = (m(0,2)*m(2,1) - m(0,1)*m(2,2)) * invdet;
    inv(0,2) = (m(0,1)*m(1,2) - m(0,2)*m(1,1)) * invdet;

    inv(1,0) = (m(1,2)*m(2,0) - m(1,0)*m(2,2)) * invdet;
    inv(1,1) = (m(0,0)*m(2,2) - m(0,2)*m(2,0)) * invdet;
    inv(1,2) = (m(0,2)*m(1,0) - m(0,0)*m(1,2)) * invdet;

    inv(2,0) = (m(1,0)*m(2,1) - m(1,1)*m(2,0)) * invdet;
    inv(2,1) = (m(0,1)*m(2,0) - m(0,0)*m(2,1)) * invdet;
    inv(2,2) = (m(0,0)*m(1,1) - m(0,1)*m(1,0)) * invdet;

    return inv;
}
