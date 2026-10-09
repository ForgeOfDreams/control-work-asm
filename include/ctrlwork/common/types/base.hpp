#ifndef CTRLWORK_COMMON_BASE_TYPES_HPP
#define CTRLWORK_COMMON_BASE_TYPES_HPP

#include <cstdint>
#include <string_view>

namespace ctrlwork {

// Коды возврата asm-ядер; значения фиксированы ABI с FASM.
enum class AsmStatus : std::int32_t {
    SUCCESS = 0,                // успех
    ERR_DIMENSION_MISMATCH = 1, // несовместимые размеры
    ERR_NULL_POINTER = 2,       // передан нулевой указатель
    ERR_DIV_BY_ZERO = 3,        // деление на ноль
    ERR_OVERFLOW = 4,           // арифметическое переполнение

    ERR_ALIASING = 5,           // выходной буфер перекрывается со входным
    ERR_INVALID_FORMAT = 6      // некорректная строка (пустая или не-цифра)
};

// Текстовое имя статуса для сообщений об ошибках.
[[nodiscard]] constexpr std::string_view to_string(AsmStatus status) noexcept {
    switch (status) {                                                         // перебираем все коды
        case AsmStatus::SUCCESS: return "success";                            // 0
        case AsmStatus::ERR_DIMENSION_MISMATCH: return "dimension mismatch";  // 1
        case AsmStatus::ERR_NULL_POINTER: return "null pointer";              // 2
        case AsmStatus::ERR_DIV_BY_ZERO: return "division by zero";           // 3
        case AsmStatus::ERR_OVERFLOW: return "overflow";                      // 4
        case AsmStatus::ERR_ALIASING: return "overlapping buffers";           // 5
        case AsmStatus::ERR_INVALID_FORMAT: return "invalid format";          // 6
    }
    return "unknown status";  // значение вне перечисления
}

} // namespace ctrlwork

#endif // CTRLWORK_COMMON_BASE_TYPES_HPP