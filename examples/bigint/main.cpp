#include "ctrlwork/bigint/bigint.hpp"

#include <format>
#include <iostream>
#include <stdexcept>

namespace {

int failures = 0;  // число проваленных проверок

void expect(bool ok, const char* what) {                                // проверка одного условия
    std::cout << (ok ? "[ ok ] " : "[FAIL] ") << what << std::endl;     // результат в stdout
    if (!ok) {                                                          // условие не выполнено
        ++failures;                                                     // запоминаем провал
    }
}

}  // namespace


int main() {
    using ctrlwork::AsmStatus;        // код ошибки ядра
    using ctrlwork::bigint::BigInt;   // основной тип

    BigInt factorial = 1;                                          // 30! считаем умножением
    for (int i = 2; i <= 30; ++i) {                                // 2 * 3 * ... * 30
        factorial *= i;                                         // целые приводятся к BigInt неявно
    }
    expect(factorial.to_string() == "265252859812191058636308480000000", "30! (multiplication, output)");

    BigInt power = 1;                                           // 2^128 не помещается в 64 бита
    for (int i = 0; i < 128; ++i) {                             // 128 раз удваиваем
        power += power;                                         // сложение с переносом между лимбами
    }
    expect(power.to_string() == "340282366920938463463374607431768211456", "2^128 (addition with carry)");
    expect(power.limb_count() == 3, "2^128 occupies 3 limbs.");                    // 2^128 = единица в третьем лимбе

    const BigInt ten19("10000000000000000000");                                     // ввод из десятичной строки
    const auto parts = BigInt::divmod(power, ten19);          // деление без исключений
    expect(parts && parts->quotient.to_string() == "34028236692093846346" &&
               parts->remainder.to_string() == "3374607431768211456",
           "2^128 / 10^19 (quotient and remainder)");

    const BigInt a("123456789012345678901234567890");           // 30 цифр
    expect((a * a).to_string() == "15241578753238836750495351562536198787501905199875019052100", "a * a");
    expect(a / 97 == BigInt("1272750402189130710322005854") && a % 97 == 52, "a / 97, a % 97");
    expect((a / 97) * 97 + a % 97 == a, "q * b + r == a");      // тождество деления

    expect(BigInt(-7) / 2 == -3 && BigInt(-7) % 2 == -1, "division truncates towards zero");
    expect(BigInt(5) - 8 == -3 && -BigInt(0) == 0, "subtraction by sign inversion, unsigned zero");
    expect(BigInt(-5) < BigInt(3) && power > a, "comparisons");

    const BigInt power256 = power * power;                      // 2^256 = (2^128)^2
    expect(power256 / power == power && (power256 % power).is_zero(), "2^256 / 2^128 == 2^128");

    bool threw = false;                                         // деление на ноль
    try {
        (void)(a / BigInt(0));                             // бросает std::domain_error
    } catch (const std::domain_error&) {
        threw = true;                                           // ожидаемое исключение
    }
    expect(threw, "division by zero -> domain_error");

    const auto bad = BigInt::from_decimal("12x4");              // ввод без исключений
    expect(!bad && bad.error() == AsmStatus::ERR_INVALID_FORMAT, "invalid string -> ERR_INVALID_FORMAT");

    std::cout << (failures == 0 ? "All checks passed" : "Some checks FAILED") << std::endl;
    return failures == 0 ? 0 : 1;                               // код для ctest
}
