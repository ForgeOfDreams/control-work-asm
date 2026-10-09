#ifndef CTRLWORK_COMMON_BIGINT_TYPES_HPP
#define CTRLWORK_COMMON_BIGINT_TYPES_HPP

namespace ctrlwork {

// C-совместимый дескриптор беззнакового многобитного числа для asm (16 байт).
struct BigIntView {
    std::uint64_t* data;  // смещение 0x00: лимбы, младший первым
    std::uint64_t len;    // смещение 0x08: число лимбов (>= 1)
};

static_assert(std::is_standard_layout_v<BigIntView>);  // нужен для offsetof и C ABI
static_assert(sizeof(BigIntView) == 16);               // размер, ожидаемый asm
static_assert(offsetof(BigIntView, data) == 0);        // BV_DATA
static_assert(offsetof(BigIntView, len) == 8);         // BV_LEN

// Ядра на FASM: C-линковка, исключений не бросают.
extern "C" {
    // r = a + b; r.len >= max(a.len, b.len); перенос за r -> ERR_OVERFLOW.
    [[nodiscard]] AsmStatus asm_bigint_add(
        const BigIntView* a,
        const BigIntView* b,
        BigIntView* result
        ) noexcept;

    // r = a - b при a >= b; r.len >= max(a.len, b.len); a < b -> ERR_OVERFLOW.
    [[nodiscard]] AsmStatus asm_bigint_sub(
        const BigIntView* a,
        const BigIntView* b,
        BigIntView* result
        ) noexcept;

    // r = a * b; r.len >= a.len + b.len; r не перекрывается с a и b.
    [[nodiscard]] AsmStatus asm_bigint_mul(
        const BigIntView* a,
        const BigIntView* b,
        BigIntView* result
        ) noexcept;

    // q = a / b, r = a % b; q.len >= a.len, r.len >= b.len; b == 0 -> ERR_DIV_BY_ZERO.
    [[nodiscard]] AsmStatus asm_bigint_divmod(
        const BigIntView* a,
        const BigIntView* b,
        BigIntView* quotient,
        BigIntView* remainder
        ) noexcept;

    // Десятичные цифры -> число; result.len >= ceil(length / 19).
    [[nodiscard]] AsmStatus asm_bigint_from_decimal(
        const char* digits,
        std::uint64_t length,
        BigIntView* result
        ) noexcept;

    // Число -> десятичные цифры в buffer[0, *length); value разрушается; capacity >= 20 * value.len.
    [[nodiscard]] AsmStatus asm_bigint_to_decimal(
        BigIntView* value,
        char* buffer,
        std::uint64_t capacity,
        std::uint64_t* length
        ) noexcept;

}

} // namespace ctrlwork

#endif // CTRLWORK_COMMON_BIGINT_TYPES_HPP
