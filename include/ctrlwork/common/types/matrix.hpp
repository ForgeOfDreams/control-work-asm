#ifndef CTRLWORK_COMMON_MATRIX_TYPES_HPP
#define CTRLWORK_COMMON_MATRIX_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ctrlwork {

// C-совместимый дескриптор матрицы для asm (24 байта, row-major, без stride).
struct MatrixView {
    std::int32_t* data;  // смещение 0x00: буфер элементов
    std::uint64_t rows;  // смещение 0x08: число строк
    std::uint64_t cols;  // смещение 0x10: число столбцов
};

// Раскладка должна точно совпадать с MV_* в matrix_math.asm.
static_assert(std::is_standard_layout_v<MatrixView>);  // нужен для offsetof и C ABI
static_assert(sizeof(MatrixView) == 24);               // размер, ожидаемый asm
static_assert(offsetof(MatrixView, data) == 0);        // MV_DATA
static_assert(offsetof(MatrixView, rows) == 8);        // MV_ROWS
static_assert(offsetof(MatrixView, cols) == 16);       // MV_COLS

// Ядра на FASM: C-линковка, исключений не бросают.
extern "C" {

// C = A + B (поэлементно).
[[nodiscard]] AsmStatus asm_matrix_add(
    const MatrixView* a,
    const MatrixView* b,
    MatrixView* result
    ) noexcept;

// C = A - B (поэлементно).
[[nodiscard]] AsmStatus asm_matrix_sub(
    const MatrixView* a,
    const MatrixView* b,
    MatrixView* result
    ) noexcept;

// C = A * B (матричное произведение); result не должен перекрываться с a и b.
[[nodiscard]] AsmStatus asm_matrix_mul(
    const MatrixView* a,
    const MatrixView* b,
    MatrixView* result
    ) noexcept;

// B = A^T; result не должен перекрываться с a.
[[nodiscard]] AsmStatus asm_matrix_transpose(
    const MatrixView* a,
    MatrixView* result
    ) noexcept;

}  // extern "C"

} // namespace ctrlwork

#endif // CTRLWORK_COMMON_MATRIX_TYPES_HPP
