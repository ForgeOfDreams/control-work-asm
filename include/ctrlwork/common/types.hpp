#ifndef CTRLWORK_COMMON_TYPES_HPP
#define CTRLWORK_COMMON_TYPES_HPP

#include <cstdint>

/**
 * @brief Common status codes and data structures shared between C++ and FASM assembly kernels.
 */

extern "C" {

/**
 * @brief Error codes returned by FASM assembly procedures.
 */
enum class AsmStatus : int32_t {
    SUCCESS = 0,               /// Operation completed successfully
    ERR_DIMENSION_MISMATCH = 1,/// Matrix or vector dimensions are incompatible
    ERR_NULL_POINTER = 2,      /// Null pointer passed as argument
    ERR_DIV_BY_ZERO = 3,       /// Division by zero attempt
    ERR_OVERFLOW = 4           /// Arithmetic overflow detected
};

/**
 * @brief Low-level C-compatible layout for matrix descriptor passed to FASM.
 * 
 * Layout in memory (24 bytes total):
 *  - Offset 0x00: Pointer to 32-bit signed integer buffer (8 bytes)
 *  - Offset 0x08: Number of rows (8 bytes)
 *  - Offset 0x10: Number of columns (8 bytes)
 */
struct MatrixView {
    int32_t* data;
    uint64_t rows;
    uint64_t cols;
};

// FASM Assembly External Function Declarations

/**
 * @brief Computes element-wise addition: C = A + B
 * @param a Pointer to input matrix descriptor A
 * @param b Pointer to input matrix descriptor B
 * @param result Pointer to output matrix descriptor C
 * @return AsmStatus Status code
 */
AsmStatus asm_matrix_add(const MatrixView* a, const MatrixView* b, MatrixView* result);

/**
 * @brief Computes element-wise subtraction: C = A - B
 * @param a Pointer to input matrix descriptor A
 * @param b Pointer to input matrix descriptor B
 * @param result Pointer to output matrix descriptor C
 * @return AsmStatus Status code
 */
AsmStatus asm_matrix_sub(const MatrixView* a, const MatrixView* b, MatrixView* result);

/**
 * @brief Computes matrix multiplication: C = A * B
 * @param a Pointer to input matrix descriptor A
 * @param b Pointer to input matrix descriptor B
 * @param result Pointer to output matrix descriptor C
 * @return AsmStatus Status code
 */
AsmStatus asm_matrix_mul(const MatrixView* a, const MatrixView* b, MatrixView* result);

/**
 * @brief Computes matrix transposition: B = A^T
 * @param a Pointer to input matrix descriptor A
 * @param result Pointer to output matrix descriptor B
 * @return AsmStatus Status code
 */
AsmStatus asm_matrix_transpose(const MatrixView* a, MatrixView* result);

} // extern "C"

#endif // CTRLWORK_COMMON_TYPES_HPP
