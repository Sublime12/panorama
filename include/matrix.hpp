#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <vector>
#include <iostream>
#include <cmath>
#include <stdexcept>

struct Matrix {
    int rows;
    int cols;
    std::vector<double> data;

    Matrix() : rows(0), cols(0) {}
    Matrix(int r, int c) : rows(r), cols(c), data(r * c, 0.0) {}
    Matrix(int r, int c, double val) : rows(r), cols(c), data(r * c, val) {}

    static Matrix identity(int n) {
        Matrix res(n, n, 0.0);
        for (int i = 0; i < n; ++i) res(i, i) = 1.0;
        return res;
    }

    inline double& operator()(int r, int c) {
        return data[r * cols + c];
    }

    inline const double& operator()(int r, int c) const {
        return data[r * cols + c];
    }

    Matrix transpose() const {
        Matrix res(cols, rows);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                res(c, r) = (*this)(r, c);
            }
        }
        return res;
    }

    Matrix operator*(const Matrix& other) const {
        if (cols != other.rows) {
            throw std::invalid_argument("Matrix dimension mismatch in multiplication");
        }
        Matrix res(rows, other.cols, 0.0);
        for (int r = 0; r < rows; ++r) {
            for (int k = 0; k < cols; ++k) {
                double val = (*this)(r, k);
                for (int c = 0; c < other.cols; ++c) {
                    res(r, c) += val * other(k, c);
                }
            }
        }
        return res;
    }

    // Solves Ax = b where A is this matrix (rows x cols) and b is (rows x 1)
    // If overdetermined (rows > cols), uses least squares (A^T A x = A^T b)
    bool solve(const Matrix& b, Matrix& x) const;

    // 3x3 Inverse
    Matrix inv3x3() const;
};

#endif // MATRIX_HPP
