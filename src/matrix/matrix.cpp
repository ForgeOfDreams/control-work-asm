#include "ctrlwork/matrix/matrix.hpp"
#include <iomanip>
#include <stdexcept>

namespace ctrlwork::matrix {

Matrix::Matrix(uint64_t rows, uint64_t cols)
    : rows_(rows), cols_(cols), data_(rows * cols, 0) {}

Matrix::Matrix(uint64_t rows, uint64_t cols, std::initializer_list<int32_t> init)
    : rows_(rows), cols_(cols), data_(init) {
    if (data_.size() != rows * cols) {
        throw std::invalid_argument("Initializer list size does not match matrix dimensions.");
    }
}

int32_t& Matrix::at(uint64_t r, uint64_t c) {
    if (r >= rows_ || c >= cols_) {
        throw std::out_of_range("Matrix index out of bounds.");
    }
    return data_[r * cols_ + c];
}

int32_t Matrix::at(uint64_t r, uint64_t c) const {
    if (r >= rows_ || c >= cols_) {
        throw std::out_of_range("Matrix index out of bounds.");
    }
    return data_[r * cols_ + c];
}

MatrixView Matrix::view() noexcept {
    return MatrixView{data_.data(), rows_, cols_};
}

MatrixView Matrix::view() const noexcept {
    return MatrixView{const_cast<int32_t*>(data_.data()), rows_, cols_};
}

void Matrix::fill_sequence(int32_t start_value) {
    for (auto& val : data_) {
        val = start_value++;
    }
}

void Matrix::print(const std::string& title) const {
    std::cout << "--- " << title << " (" << rows_ << "x" << cols_ << ") ---\n";
    for (uint64_t i = 0; i < rows_; ++i) {
        std::cout << "[ ";
        for (uint64_t j = 0; j < cols_; ++j) {
            std::cout << std::setw(6) << at(i, j) << " ";
        }
        std::cout << "]\n";
    }
    std::cout << std::endl;
}

std::expected<Matrix, AsmStatus> Matrix::add(const Matrix& a, const Matrix& b) {
    Matrix result(a.rows(), a.cols());
    auto vA = a.view();
    auto vB = b.view();
    auto vR = result.view();

    AsmStatus status = asm_matrix_add(&vA, &vB, &vR);
    if (status == AsmStatus::SUCCESS) return result;
    return std::unexpected(status);
}

std::expected<Matrix, AsmStatus> Matrix::sub(const Matrix& a, const Matrix& b) {
    Matrix result(a.rows(), a.cols());
    auto vA = a.view();
    auto vB = b.view();
    auto vR = result.view();

    AsmStatus status = asm_matrix_sub(&vA, &vB, &vR);
    if (status == AsmStatus::SUCCESS) return result;
    return std::unexpected(status);
}

std::expected<Matrix, AsmStatus> Matrix::mul(const Matrix& a, const Matrix& b) {
    Matrix result(a.rows(), b.cols());
    auto vA = a.view();
    auto vB = b.view();
    auto vR = result.view();

    AsmStatus status = asm_matrix_mul(&vA, &vB, &vR);
    if (status == AsmStatus::SUCCESS) return result;
    return std::unexpected(status);
}

std::expected<Matrix, AsmStatus> Matrix::transpose(const Matrix& a) {
    Matrix result(a.cols(), a.rows());
    auto vA = a.view();
    auto vR = result.view();

    AsmStatus status = asm_matrix_transpose(&vA, &vR);
    if (status == AsmStatus::SUCCESS) return result;
    return std::unexpected(status);
}

} // namespace ctrlwork::matrix
