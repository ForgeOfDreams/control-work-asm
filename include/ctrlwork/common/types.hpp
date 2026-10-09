#ifndef CTRLWORK_COMMON_TYPES_HPP
#define CTRLWORK_COMMON_TYPES_HPP

#include <cstddef>      // offsetof
#include <cstdint>      // std::int32_t, std::uint64_t
#include <string_view>  // to_string()
#include <type_traits>  // std::is_standard_layout_v

namespace ctrlwork {  // всё общее живёт в namespace, а не в глобальной области

// Коды возврата asm-ядер; значения фиксированы ABI с FASM.
enum class AsmStatus : std::int32_t {
    SUCCESS = 0,                // успех
    ERR_DIMENSION_MISMATCH = 1, // несовместимые размеры
    ERR_NULL_POINTER = 2,       // передан нулевой указатель
    ERR_DIV_BY_ZERO = 3,        // деление на ноль
    ERR_OVERFLOW = 4            // арифметическое переполнение
};

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

// Текстовое имя статуса для сообщений об ошибках.
[[nodiscard]] constexpr std::string_view to_string(AsmStatus status) noexcept {
    switch (status) {                                           // перебираем все коды
        case AsmStatus::SUCCESS: return "success";              // 0
        case AsmStatus::ERR_DIMENSION_MISMATCH: return "dimension mismatch";  // 1
        case AsmStatus::ERR_NULL_POINTER: return "null pointer";              // 2
        case AsmStatus::ERR_DIV_BY_ZERO: return "division by zero";           // 3
        case AsmStatus::ERR_OVERFLOW: return "overflow";                      // 4
    }
    return "unknown status";  // значение вне перечисления
}

// Ядра на FASM: C-линковка (имена без манглинга), исключений не бросают.
extern "C" {
// C = A + B (поэлементно).
[[nodiscard]] AsmStatus asm_matrix_add(const MatrixView* a, const MatrixView* b,
                                       MatrixView* result) noexcept;
// C = A - B (поэлементно).
[[nodiscard]] AsmStatus asm_matrix_sub(const MatrixView* a, const MatrixView* b,
                                       MatrixView* result) noexcept;
// C = A * B (матричное произведение); result не должен перекрываться с a и b.
[[nodiscard]] AsmStatus asm_matrix_mul(const MatrixView* a, const MatrixView* b,
                                       MatrixView* result) noexcept;
// B = A^T; result не должен перекрываться с a.
[[nodiscard]] AsmStatus asm_matrix_transpose(const MatrixView* a, MatrixView* result) noexcept;
}  // extern "C"

}  // namespace ctrlwork

#endif  // CTRLWORK_COMMON_TYPES_HPP
