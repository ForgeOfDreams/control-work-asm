#ifndef CTRLWORK_MATRIX_MATRIX_HPP
#define CTRLWORK_MATRIX_MATRIX_HPP

#include "ctrlwork/common/types.hpp"
#include "ctrlwork/matrix/export.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <iosfwd>
#include <span>
#include <string_view>
#include <vector>

namespace ctrlwork::matrix {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)  // std::vector в экспортируемом классе: он приватный
#endif

// Плотная матрица int32, row-major; вычисления выполняют FASM-ядра.
// Инвариант: data_.size() == rows_ * cols_ (в том числе после перемещения).
class CTRLWORK_MATRIX_EXPORT Matrix {
public:
    using value_type = std::int32_t;  // тип элемента
    using size_type = std::uint64_t;  // тип размеров (как в MatrixView)

    Matrix() noexcept = default;                     // пустая матрица 0x0
    Matrix(size_type rows, size_type cols);          // нулевая матрица; throws length_error
    Matrix(size_type rows, size_type cols,
           std::initializer_list<value_type> init);  // значения построчно; throws invalid_argument

    Matrix(const Matrix&) = default;                 // копирование: глубокое
    Matrix& operator=(const Matrix&) = default;      // присваивание копированием
    Matrix(Matrix&& other) noexcept;                 // перемещение: источник становится 0x0
    Matrix& operator=(Matrix&& other) noexcept;      // присваивание перемещением
    ~Matrix() = default;                             // освобождает vector

    [[nodiscard]] size_type rows() const noexcept { return rows_; }  // число строк
    [[nodiscard]] size_type cols() const noexcept { return cols_; }  // число столбцов
    [[nodiscard]] size_type size() const noexcept { return data_.size(); }  // число элементов
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }     // нет элементов

    // Доступ без проверки границ (проверяется только assert в Debug).
    [[nodiscard]] value_type& operator()(size_type row, size_type col) noexcept {
        assert(row < rows_ && col < cols_ && "Matrix index out of range");  // контракт вызова
        return data_[row * cols_ + col];                                    // индекс row-major
    }
    [[nodiscard]] const value_type& operator()(size_type row, size_type col) const noexcept {
        assert(row < rows_ && col < cols_ && "Matrix index out of range");  // контракт вызова
        return data_[row * cols_ + col];                                    // индекс row-major
    }

    [[nodiscard]] value_type& at(size_type row, size_type col);              // с проверкой границ
    [[nodiscard]] const value_type& at(size_type row, size_type col) const;  // throws out_of_range

    // Все элементы подряд (row-major) как span.
    [[nodiscard]] std::span<value_type> elements() noexcept { return data_; }
    [[nodiscard]] std::span<const value_type> elements() const noexcept { return data_; }

    [[nodiscard]] MatrixView view() noexcept;              // дескриптор для asm (чтение/запись)
    [[nodiscard]] MatrixView view() const noexcept;        // дескриптор для asm (только чтение)

    void fill_sequence(value_type start_value = 1) noexcept;  // start, start+1, ... (с переносом)

    void print(std::string_view title) const;                       // печать в std::cout
    void print(std::string_view title, std::ostream& os) const;     // печать в произвольный поток

    // Операторы бросают std::invalid_argument при несовместимых размерах.
    [[nodiscard]] Matrix operator+(const Matrix& other) const;  // поэлементная сумма
    [[nodiscard]] Matrix operator-(const Matrix& other) const;  // поэлементная разность
    [[nodiscard]] Matrix operator*(const Matrix& other) const;  // матричное произведение
    [[nodiscard]] Matrix transposed() const;                    // транспонированная копия

    [[nodiscard]] bool operator==(const Matrix&) const = default;  // размеры и все элементы

    // API без исключений: результат либо Matrix, либо код ошибки ядра.
    [[nodiscard]] static std::expected<Matrix, AsmStatus> add(const Matrix& a, const Matrix& b);
    [[nodiscard]] static std::expected<Matrix, AsmStatus> sub(const Matrix& a, const Matrix& b);
    [[nodiscard]] static std::expected<Matrix, AsmStatus> mul(const Matrix& a, const Matrix& b);
    [[nodiscard]] static std::expected<Matrix, AsmStatus> transpose(const Matrix& a);

private:
    static std::size_t checked_size(size_type rows, size_type cols);  // rows*cols или length_error
    void check_bounds(size_type row, size_type col) const;            // out_of_range при выходе

    size_type rows_{0};            // число строк (объявлено раньше data_: порядок инициализации)
    size_type cols_{0};            // число столбцов
    std::vector<value_type> data_; // элементы построчно
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// CTRLWORK_MATRIX_EXPORT std::ostream& operator<<(std::ostream& os, const Matrix& m);  // вывод строк

}  // namespace ctrlwork::matrix

#endif  // CTRLWORK_MATRIX_MATRIX_HPP
