#ifndef CTRLWORK_MATRIX_MATRIX_HPP
#define CTRLWORK_MATRIX_MATRIX_HPP

#include "ctrlwork/common/types.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <expected>

namespace ctrlwork::matrix {

/**
 * @class Matrix
 * @brief High-level C++ RAII wrapper over matrix buffers, interfacing with FASM assembly kernel.
 */
class Matrix {
private:
    std::vector<int32_t> data_;
    uint64_t rows_;
    uint64_t cols_;

public:
    /**
     * @brief Constructs a matrix with given dimensions initialized to zeros.
     */
    Matrix(uint64_t rows, uint64_t cols);

    /**
     * @brief Constructs a matrix initialized with a flattened initializer list.
     */
    Matrix(uint64_t rows, uint64_t cols, std::initializer_list<int32_t> init);

    [[nodiscard]] uint64_t rows() const noexcept { return rows_; }
    [[nodiscard]] uint64_t cols() const noexcept { return cols_; }
    
    /**
     * @brief Provides element access with boundary check.
     */
    int32_t& at(uint64_t r, uint64_t c);
    [[nodiscard]] int32_t at(uint64_t r, uint64_t c) const;

    /**
     * @brief Generates a C-compatible view for FASM procedures.
     */
    [[nodiscard]] MatrixView view() noexcept;
    [[nodiscard]] MatrixView view() const noexcept;

    /**
     * @brief Fills the matrix sequentially starting from start_value.
     */
    void fill_sequence(int32_t start_value = 1);

    /**
     * @brief Formats and prints matrix contents to stdout.
     */
    void print(const std::string& title) const;

    // Static mathematical wrappers calling FASM kernel
    static std::expected<Matrix, AsmStatus> add(const Matrix& a, const Matrix& b);
    static std::expected<Matrix, AsmStatus> sub(const Matrix& a, const Matrix& b);
    static std::expected<Matrix, AsmStatus> mul(const Matrix& a, const Matrix& b);
    static std::expected<Matrix, AsmStatus> transpose(const Matrix& a);
};

} // namespace ctrlwork::matrix

#endif // CTRLWORK_MATRIX_MATRIX_HPP
