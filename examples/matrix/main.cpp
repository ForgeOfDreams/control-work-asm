#include <iostream>
#include <iomanip>

#include "ctrlwork/matrix/matrix.hpp"

void print_matrix(const char* name, const ctrlwork::matrix::Matrix& mat) {
    std::cout << "--- Matrix " << name << " (" << mat.rows() << "x" << mat.cols() << ") ---\n";
    for (uint64_t i = 0; i < mat.rows(); ++i) {
        for (uint64_t j = 0; j < mat.cols(); ++j) {
            std::cout << std::setw(6) << mat.at(i, j) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main () {
    try {
        ctrlwork::matrix::Matrix A(2, 3);
        ctrlwork::matrix::Matrix B(2, 3);

        A.at(0, 0) = 5; A.at(0, 1) = 8; A.at(0, 2) = 3;
        A.at(1, 0) = 1; A.at(1, 1) = 2; A.at(1, 2) = 9;

        B.at(0, 0) = 1; B.at(0, 1) = 2; B.at(0, 2) = 3;
        B.at(1, 0) = 4; B.at(1, 1) = 5; B.at(1, 2) = 6;

        print_matrix("A", A);
        print_matrix("B", B);

    } catch (const std::exception& e) {
        std::cerr << "[EXCEPTION CAUGHT]: " << e.what() << "\n";
    }

    return 0;
}
