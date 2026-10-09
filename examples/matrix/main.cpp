#include <ctrlwork/matrix/matrix.hpp>

#include <iostream>
#include <stdexcept>

namespace {

int failures = 0;  // число проваленных проверок

void expect(bool ok, const char* what) {
    std::cout << (ok ? "[ ok ] " : "[FAIL] ") << what << '\n';
    if (!ok) {
        ++failures;
    }
}

template <typename Exception, typename F>
bool throws(F&& f) {
    try {
        f();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    using ctrlwork::AsmStatus;          // код ошибки ядра
    using ctrlwork::matrix::Matrix;     // основной тип

    const Matrix a(2, 3, {1, 2, 3, 4, 5, 6});        // A: 2x3
    const Matrix b(3, 2, {7, 8, 9, 10, 11, 12});     // B: 3x2

    a.print("A");
    b.print("B");

    const Matrix sum = a + a;                                   // поэлементная сумма
    sum.print("A + A");
    expect(sum == Matrix(2, 3, {2, 4, 6, 8, 10, 12}), "operator+");
    expect(sum - a == a, "operator-");                  // (A + A) - A == A

    const Matrix product = a * b;                               // произведение 2x2
    expect(product == Matrix(2, 2, {58, 64, 139, 154}), "operator*");
    expect(a.transposed() == Matrix(3, 2, {1, 4, 2, 5, 3, 6}), "transposed");

    Matrix m(2, 2);                                            // нулевая 2x2
    m(0, 1) = 5;                                                // запись через operator()
    expect(m.at(0, 1) == 5 && m(1, 1) == 0, "operator() / at()");
    expect(throws<std::out_of_range>([&] { (void)m.at(2, 0); }), "at(): out_of_range");
    expect(throws<std::invalid_argument>([&] { (void)(a * a); }), "operator*: shape mismatch");

    const auto bad = Matrix::add(a, b);                         // API без исключений
    expect(!bad && bad.error() == AsmStatus::ERR_DIMENSION_MISMATCH, "add(): expected error");

    const Matrix empty;                                                 // пустая 0x0
    expect((empty + empty).empty(), "empty + empty");           // пустые матрицы допустимы

    product.print("A * B");                                         // красивый вывод
    std::cout << (failures == 0 ? "All checks passed" : "Some checks FAILED") << '\n';

    return failures == 0 ? 0 : 1;                                       // код для ctest
}
