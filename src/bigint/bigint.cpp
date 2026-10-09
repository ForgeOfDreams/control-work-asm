#include "ctrlwork/bigint/bigint.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <utility>

namespace ctrlwork::bigint {

namespace {

using Limbs = std::vector<std::uint64_t>;  // модуль числа (как BigInt::Limbs)

std::uint64_t zero_limb = 0;  // лимб-ноль: ядрам нужна длина >= 1, а у числа 0 вектор пуст

// Дескриптор входного числа; ноль отдаём как один нулевой лимб. Ядра входы не пишут.
BigIntView view_of(const Limbs& limbs) noexcept {
    if (limbs.empty()) {                                                       // число 0
        return BigIntView{&zero_limb, 1};                             // один лимб = 0
    }
    return BigIntView{const_cast<std::uint64_t*>(limbs.data()), limbs.size()};  // const снимаем для ABI
}

// Дескриптор выходного буфера (всегда непустой).
BigIntView view_of(Limbs& limbs) noexcept {
    return BigIntView{limbs.data(), limbs.size()};  // указатель и длина
}

// Убирает старшие нулевые лимбы (ноль -> пустой вектор).
void trim(Limbs& limbs) noexcept {
    while (!limbs.empty() && limbs.back() == 0) {  // пока старший лимб нулевой
        limbs.pop_back();                          // отбрасываем его
    }
}

// Сравнение модулей: сначала по длине, затем со старшего лимба.
std::strong_ordering compare_magnitude(const Limbs& a, const Limbs& b) noexcept {
    if (a.size() != b.size()) {                    // разная длина решает сразу
        return a.size() <=> b.size();              // длиннее - больше (модули нормализованы)
    }
    for (std::size_t i = a.size(); i-- > 0;) {     // от старшего лимба к младшему
        if (a[i] != b[i]) {                        // первое различие
            return a[i] <=> b[i];                  // решает порядок
        }
    }
    return std::strong_ordering::equal;            // все лимбы равны
}

// Бросает исключение, соответствующее статусу ядра.
[[noreturn]] void throw_status(AsmStatus status) {
    auto message = std::format("BigInt operation failed: {}", to_string(status));  // текст ошибки
    switch (status) {                                                              // выбор типа
        case AsmStatus::ERR_DIV_BY_ZERO:                                           // деление на ноль
            throw std::domain_error(std::move(message));
        case AsmStatus::ERR_INVALID_FORMAT:                                        // плохая строка
            throw std::invalid_argument(std::move(message));
        default:                                                                   // прочее - ошибка ядра
            throw std::runtime_error(std::move(message));
    }
}

// Возвращает значение или бросает исключение по коду ошибки.
template <typename T>
T unwrap(std::expected<T, AsmStatus>&& result) {
    if (!result) {                   // есть ошибка
        throw_status(result.error());  // преобразуем код в исключение
    }
    return *std::move(result);       // забираем значение без копирования
}

// |a| + |b|.
std::expected<Limbs, AsmStatus> mag_add(const Limbs& a, const Limbs& b) {
    const BigIntView va = view_of(a);                                   // дескриптор a
    const BigIntView vb = view_of(b);                                   // дескриптор b
    Limbs result(static_cast<std::size_t>(std::max(va.len, vb.len)) + 1);  // +1 лимб под перенос
    BigIntView vr = view_of(result);                                    // дескриптор результата
    if (const AsmStatus status = asm_bigint_add(&va, &vb, &vr); status != AsmStatus::SUCCESS) {
        return std::unexpected(status);                                 // ошибка ядра
    }
    trim(result);                                                       // убираем лишний старший лимб
    return result;                                                      // готовый модуль
}

// |a| - |b|, требуется |a| >= |b|.
std::expected<Limbs, AsmStatus> mag_sub(const Limbs& a, const Limbs& b) {
    const BigIntView va = view_of(a);                                   // дескриптор a
    const BigIntView vb = view_of(b);                                   // дескриптор b
    Limbs result(static_cast<std::size_t>(std::max(va.len, vb.len)));   // разность не длиннее
    BigIntView vr = view_of(result);                                    // дескриптор результата
    if (const AsmStatus status = asm_bigint_sub(&va, &vb, &vr); status != AsmStatus::SUCCESS) {
        return std::unexpected(status);                                 // ошибка ядра
    }
    trim(result);                                                       // убираем старшие нули
    return result;                                                      // готовый модуль
}

// |a| * |b|.
std::expected<Limbs, AsmStatus> mag_mul(const Limbs& a, const Limbs& b) {
    const BigIntView va = view_of(a);                                      // дескриптор a
    const BigIntView vb = view_of(b);                                      // дескриптор b
    Limbs result(static_cast<std::size_t>(va.len + vb.len));               // произведение <= LA + LB лимбов
    BigIntView vr = view_of(result);                                    // дескриптор результата
    if (const AsmStatus status = asm_bigint_mul(&va, &vb, &vr); status != AsmStatus::SUCCESS) {
        return std::unexpected(status);                                 // ошибка ядра
    }
    trim(result);                                                       // убираем старшие нули
    return result;                                                      // готовый модуль
}

// (|a| / |b|, |a| % |b|); b != 0.
std::expected<std::pair<Limbs, Limbs>, AsmStatus> mag_divmod(const Limbs& a, const Limbs& b) {
    const BigIntView va = view_of(a);                                      // дескриптор a
    const BigIntView vb = view_of(b);                                      // дескриптор b
    Limbs quotient(static_cast<std::size_t>(va.len));                      // частное <= LA лимбов
    Limbs remainder(static_cast<std::size_t>(vb.len));                     // остаток < b: <= LB лимбов
    BigIntView vq = view_of(quotient);                                  // дескриптор частного
    BigIntView vr = view_of(remainder);                                 // дескриптор остатка
    if (const AsmStatus status = asm_bigint_divmod(&va, &vb, &vq, &vr); status != AsmStatus::SUCCESS) {
        return std::unexpected(status);                                 // ошибка ядра
    }
    trim(quotient);                                                     // нормализуем частное
    trim(remainder);                                                    // нормализуем остаток
    return std::pair{std::move(quotient), std::move(remainder)};        // оба модуля
}

}  // namespace

BigInt::BigInt(Limbs&& limbs, bool negative)
    : limbs_(std::move(limbs)), negative_(negative && !limbs_.empty()) {}  // у нуля знак всегда "+"

BigInt::BigInt(std::string_view decimal) : BigInt(unwrap(from_decimal(decimal))) {}  // или исключение

void BigInt::assign(std::uint64_t magnitude, bool negative) {
    limbs_.clear();                         // сбрасываем прежнее значение
    if (magnitude != 0) {                   // ненулевой модуль
        limbs_.push_back(magnitude);        // один лимб
    }
    negative_ = negative && magnitude != 0; // у нуля знак "+"
}

std::expected<BigInt, AsmStatus> BigInt::from_decimal(std::string_view text) {
    bool negative = false;                                      // знак числа
    if (!text.empty() && (text.front() == '-' || text.front() == '+')) {  // есть знак
        negative = text.front() == '-';                         // запоминаем знак
        text.remove_prefix(1);                                  // оставляем только цифры
    }
    if (text.empty()) {                                         // нет ни одной цифры
        return std::unexpected(AsmStatus::ERR_INVALID_FORMAT);  // формат неверен
    }
    Limbs limbs((text.size() + 18) / 19);                       // ceil(n/19) лимбов всегда хватает
    BigIntView view = view_of(limbs);                           // дескриптор результата
    if (const AsmStatus status = asm_bigint_from_decimal(text.data(), text.size(), &view);
        status != AsmStatus::SUCCESS) {
        return std::unexpected(status);                         // нецифра и т.п.
    }
    trim(limbs);                                                // убираем старшие нули
    return BigInt(std::move(limbs), negative);                  // с учётом знака
}

std::string BigInt::to_string() const {
    if (limbs_.empty()) {                                       // ноль
        return "0";                                             // единственная цифра
    }
    Limbs work = limbs_;                                        // ядро разрушает вход - даём копию
    BigIntView view = view_of(work);                            // дескриптор копии
    std::string digits(static_cast<std::size_t>(view.len) * 20, '\0');  // 20 цифр на лимб достаточно
    std::uint64_t length = 0;                                   // сколько цифр записало ядро
    if (const AsmStatus status = asm_bigint_to_decimal(&view, digits.data(), digits.size(), &length);
        status != AsmStatus::SUCCESS) {
        throw_status(status);                                   // не должно случиться
    }
    digits.resize(static_cast<std::size_t>(length));            // обрезаем до реальной длины
    if (negative_) {                                            // отрицательное число
        digits.insert(digits.begin(), '-');                     // добавляем знак
    }
    return digits;                                              // десятичная запись
}

BigInt BigInt::operator-() const {
    return BigInt(Limbs(limbs_), !negative_);  // тот же модуль, обратный знак (у нуля остаётся "+")
}

BigInt BigInt::abs() const {
    return BigInt(Limbs(limbs_), false);       // тот же модуль, знак "+"
}

std::strong_ordering BigInt::operator<=>(const BigInt& other) const {
    if (negative_ != other.negative_) {                       // разные знаки решают сразу
        return negative_ ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    const auto magnitude = compare_magnitude(limbs_, other.limbs_);  // сравнение модулей
    return negative_ ? 0 <=> magnitude : magnitude;           // у отрицательных порядок обратный
}

std::expected<BigInt, AsmStatus> BigInt::add_signed(const BigInt& a, const Limbs& b_limbs,
                                                    bool b_negative) {
    if (a.negative_ == b_negative) {                          // одинаковые знаки: складываем модули
        auto sum = mag_add(a.limbs_, b_limbs);                // |a| + |b|
        if (!sum) {                                           // ошибка ядра
            return std::unexpected(sum.error());
        }
        return BigInt(std::move(*sum), a.negative_);          // знак общий
    }
    const auto order = compare_magnitude(a.limbs_, b_limbs);  // разные знаки: вычитаем меньший из большего
    if (order == 0) {                                         // модули равны
        return BigInt();                                      // результат 0
    }
    if (order > 0) {                                          // |a| > |b|
        auto diff = mag_sub(a.limbs_, b_limbs);               // |a| - |b|
        if (!diff) {                                          // ошибка ядра
            return std::unexpected(diff.error());
        }
        return BigInt(std::move(*diff), a.negative_);         // знак как у a
    }
    auto diff = mag_sub(b_limbs, a.limbs_);                   // |b| - |a|
    if (!diff) {                                              // ошибка ядра
        return std::unexpected(diff.error());
    }
    return BigInt(std::move(*diff), b_negative);              // знак как у b
}

std::expected<BigInt, AsmStatus> BigInt::add(const BigInt& a, const BigInt& b) {
    return add_signed(a, b.limbs_, b.negative_);   // a + b
}

std::expected<BigInt, AsmStatus> BigInt::sub(const BigInt& a, const BigInt& b) {
    return add_signed(a, b.limbs_, !b.negative_);  // a - b = a + (-b)
}

std::expected<BigInt, AsmStatus> BigInt::mul(const BigInt& a, const BigInt& b) {
    if (a.is_zero() || b.is_zero()) {                         // умножение на ноль
        return BigInt();                                      // без вызова ядра
    }
    auto product = mag_mul(a.limbs_, b.limbs_);               // |a| * |b|
    if (!product) {                                           // ошибка ядра
        return std::unexpected(product.error());
    }
    return BigInt(std::move(*product), a.negative_ != b.negative_);  // знак - исключающее ИЛИ
}

std::expected<DivResult, AsmStatus> BigInt::divmod(const BigInt& a, const BigInt& b) {
    if (b.is_zero()) {                                        // деление на ноль
        return std::unexpected(AsmStatus::ERR_DIV_BY_ZERO);
    }
    if (compare_magnitude(a.limbs_, b.limbs_) < 0) {                             // |a| < |b|
        return DivResult{BigInt(), a};                           // частное 0, остаток a
    }
    auto parts = mag_divmod(a.limbs_, b.limbs_);    // (|a| / |b|, |a| % |b|)
    if (!parts) {                                                                       // ошибка ядра
        return std::unexpected(parts.error());
    }
    return DivResult{
        BigInt(
            std::move(parts->first),
            a.negative_ != b.negative_
        ),  // знак частного
BigInt(std::move(parts->second), a.negative_)};               // знак остатка = знак a
}

BigInt& BigInt::operator+=(const BigInt& other) { return *this = *this + other; }  // через operator+
BigInt& BigInt::operator-=(const BigInt& other) { return *this = *this - other; }  // через operator-
BigInt& BigInt::operator*=(const BigInt& other) { return *this = *this * other; }  // через operator*
BigInt& BigInt::operator/=(const BigInt& other) { return *this = *this / other; }  // через operator/
BigInt& BigInt::operator%=(const BigInt& other) { return *this = *this % other; }  // через operator%

BigInt operator+(const BigInt& a, const BigInt& b) { return unwrap(BigInt::add(a, b)); }  // сумма
BigInt operator-(const BigInt& a, const BigInt& b) { return unwrap(BigInt::sub(a, b)); }  // разность
BigInt operator*(const BigInt& a, const BigInt& b) { return unwrap(BigInt::mul(a, b)); }  // произведение

BigInt operator/(const BigInt& a, const BigInt& b) {
    return unwrap(BigInt::divmod(a, b)).quotient;   // частное или domain_error
}

BigInt operator%(const BigInt& a, const BigInt& b) {
    return unwrap(BigInt::divmod(a, b)).remainder;  // остаток или domain_error
}

}  // namespace ctrlwork::bigint
