#ifndef CTRLWORK_BIGINT_BIGINT_HPP
#define CTRLWORK_BIGINT_BIGINT_HPP

#include "ctrlwork/common/types.hpp"
#include "ctrlwork/bigint/export.h"

#include <compare>
#include <concepts>
#include <cstdint>
#include <expected>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ctrlwork::bigint {

struct DivResult;  // частное и остаток (определена после BigInt)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)  // std::vector в экспортируемом классе: он приватный
#endif

// Знаковое целое произвольной разрядности (знак + модуль), вычисления выполняют FASM-ядра.
// limbs_ без старших нулевых лимбов; пустой limbs_ == 0; у нуля negative_ == false.
class CTRLWORK_BIGINT_EXPORT BigInt {
public:
    using limb_type = std::uint64_t;  // лимб: 64 бита, младший первым

    BigInt() noexcept = default;  // ноль (после std::move источник тоже ноль)

    // Неявно из любого целого до 64 бит (кроме bool): BigInt x = 5; x + 1.
    template <std::integral T>
    requires(!std::same_as<T, bool> && sizeof(T) <= sizeof(std::uint64_t))
    BigInt(T value) {                                                   // NOLINT(google-explicit-constructor)
        if constexpr (std::is_signed_v<T>) {                            // знаковый тип
            const bool negative = value < 0;                            // знак значения
            assign(negative ? 0 - static_cast<std::uint64_t>(value)     // модуль (верно и для INT64_MIN)
                            : static_cast<std::uint64_t>(value),
                   negative);
        } else {                                                        // беззнаковый тип
            assign(static_cast<std::uint64_t>(value), false);   // модуль = значение
        }
    }

    explicit BigInt(std::string_view decimal);  // "-123"; throws invalid_argument

    // Разбор десятичной строки: [+-]цифры, без пробелов. Ошибка: ERR_INVALID_FORMAT.
    [[nodiscard]] static std::expected<BigInt, AsmStatus> from_decimal(std::string_view text);
    [[nodiscard]] std::string to_string() const;                    // десятичная запись со знаком

    [[nodiscard]] bool is_zero() const noexcept { return limbs_.empty(); }              // значение == 0
    [[nodiscard]] bool is_negative() const noexcept { return negative_; }               // значение < 0
    [[nodiscard]] std::size_t limb_count() const noexcept { return limbs_.size(); }     // число лимбов
    [[nodiscard]] std::span<const limb_type> limbs() const noexcept { return limbs_; }  // модуль

    [[nodiscard]] BigInt operator-() const;                         // смена знака
    [[nodiscard]] BigInt operator+() const { return *this; }     // унарный плюс
    [[nodiscard]] BigInt abs() const;                               // модуль

    BigInt& operator+=(const BigInt& other);  // a = a + other
    BigInt& operator-=(const BigInt& other);  // a = a - other
    BigInt& operator*=(const BigInt& other);  // a = a * other
    BigInt& operator/=(const BigInt& other);  // a = a / other (усечение к нулю)
    BigInt& operator%=(const BigInt& other);  // a = a % other (знак как у делимого)

    [[nodiscard]] bool operator==(const BigInt&) const = default;       // знак и модуль
    [[nodiscard]] std::strong_ordering operator<=>(const BigInt& other) const;  // полный порядок

    // API без исключений: результат либо BigInt, либо код ошибки ядра.
    [[nodiscard]] static std::expected<BigInt, AsmStatus> add(const BigInt& a, const BigInt& b);
    [[nodiscard]] static std::expected<BigInt, AsmStatus> sub(const BigInt& a, const BigInt& b);
    [[nodiscard]] static std::expected<BigInt, AsmStatus> mul(const BigInt& a, const BigInt& b);
    // Деление с усечением к нулю (как в C++): a == q*b + r, |r| < |b|, знак r как у a.
    [[nodiscard]] static std::expected<DivResult, AsmStatus> divmod(const BigInt& a, const BigInt& b);

private:
    using Limbs = std::vector<limb_type>;  // тип хранилища модуля

    BigInt(Limbs&& limbs, bool negative);               // limbs уже нормализован
    void assign(std::uint64_t magnitude, bool negative);  // задать из одного лимба
    [[nodiscard]] static std::expected<BigInt, AsmStatus> add_signed(
        const BigInt& a, const Limbs& b_limbs, bool b_negative);  // a + (±b)

    Limbs limbs_;           // модуль, little-endian по лимбам
    bool negative_{false};  // знак (после limbs_: порядок инициализации)
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Результат деления.
struct DivResult {
    BigInt quotient;   // частное
    BigInt remainder;  // остаток
};

// Симметричные операторы: допускают целое слева (1 + x). Бросают исключения (см. ниже).
//   std::domain_error - деление на ноль;  std::runtime_error - внутренняя ошибка ядра.
[[nodiscard]] CTRLWORK_BIGINT_EXPORT BigInt operator+(const BigInt& a, const BigInt& b);
[[nodiscard]] CTRLWORK_BIGINT_EXPORT BigInt operator-(const BigInt& a, const BigInt& b);
[[nodiscard]] CTRLWORK_BIGINT_EXPORT BigInt operator*(const BigInt& a, const BigInt& b);
[[nodiscard]] CTRLWORK_BIGINT_EXPORT BigInt operator/(const BigInt& a, const BigInt& b);
[[nodiscard]] CTRLWORK_BIGINT_EXPORT BigInt operator%(const BigInt& a, const BigInt& b);

}  // namespace ctrlwork::bigint

#endif  // CTRLWORK_BIGINT_BIGINT_HPP
