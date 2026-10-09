#include "ctrlwork/matrix/matrix.hpp"

#include <algorithm>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ctrlwork::matrix {

namespace {

static_assert(sizeof(std::size_t) == sizeof(std::uint64_t));  // asm адресует 64-битными индексами

// Бросает исключение, соответствующее статусу ядра.
[[noreturn]] void throw_status(AsmStatus status) {
    auto message = std::format("Matrix operation failed: {}", to_string(status));  // текст ошибки
    if (status == AsmStatus::ERR_DIMENSION_MISMATCH) {                             // ошибка вызывающего
        throw std::invalid_argument(std::move(message));                           // неверные размеры
    }
    throw std::runtime_error(std::move(message));  // прочие статусы: внутренняя ошибка
}

// Возвращает Matrix или бросает исключение по коду ошибки.
Matrix unwrap(std::expected<Matrix, AsmStatus>&& result) {
    if (!result) {                   // есть ошибка
        throw_status(result.error());  // преобразуем код в исключение
    }
    return *std::move(result);       // забираем значение без копирования
}

// Превращает статус ядра и готовую матрицу в std::expected.
std::expected<Matrix, AsmStatus> finish(Matrix&& result, AsmStatus status) {
    if (status != AsmStatus::SUCCESS) {  // ядро сообщило об ошибке
        return std::unexpected(status);  // отдаём код ошибки
    }
    return std::move(result);            // отдаём вычисленную матрицу
}

}  // namespace

std::size_t Matrix::checked_size(size_type rows, size_type cols) {
    if (rows != 0 && cols > std::numeric_limits<std::size_t>::max() / rows) {  // rows*cols переполнит
        throw std::length_error("Matrix dimensions overflow.");                // слишком большая матрица
    }
    return static_cast<std::size_t>(rows * cols);  // безопасное число элементов
}

void Matrix::check_bounds(size_type row, size_type col) const {
    if (row >= rows_ || col >= cols_) {                              // индекс вне матрицы
        throw std::out_of_range(std::format(                         // сообщение с индексами
            "Matrix index ({}, {}) out of bounds for {}x{}.", row, col, rows_, cols_));
    }
}

Matrix::Matrix(size_type rows, size_type cols)
    : rows_(rows), cols_(cols), data_(checked_size(rows, cols), 0) {}  // нули; размер проверен

Matrix::Matrix(size_type rows, size_type cols, std::initializer_list<value_type> init)
    : Matrix(rows, cols) {                                   // выделяем нулевую матрицу
    if (init.size() != data_.size()) {                       // список не подходит по размеру
        throw std::invalid_argument(std::format(             // сообщение с числами
            "Initializer list has {} elements, expected {}.", init.size(), data_.size()));
    }
    std::ranges::copy(init, data_.begin());                  // копируем значения построчно
}

Matrix::Matrix(Matrix&& other) noexcept
    : rows_(std::exchange(other.rows_, 0)),  // забираем строки, источнику ставим 0
      cols_(std::exchange(other.cols_, 0)),  // забираем столбцы, источнику ставим 0
      data_(std::move(other.data_)) {        // забираем буфер
    other.data_.clear();                     // гарантируем валидный пустой источник
}

Matrix& Matrix::operator=(Matrix&& other) noexcept {
    if (this != &other) {                           // защита от самоприсваивания
        rows_ = std::exchange(other.rows_, 0);      // забираем строки
        cols_ = std::exchange(other.cols_, 0);      // забираем столбцы
        data_ = std::move(other.data_);             // забираем буфер
        other.data_.clear();                        // источник остаётся валидной 0x0
    }
    return *this;                                   // для цепочек присваиваний
}

Matrix::value_type& Matrix::at(size_type row, size_type col) {
    check_bounds(row, col);     // бросает при выходе за границы
    return (*this)(row, col);   // индекс уже проверен
}

const Matrix::value_type& Matrix::at(size_type row, size_type col) const {
    check_bounds(row, col);     // бросает при выходе за границы
    return (*this)(row, col);   // индекс уже проверен
}

MatrixView Matrix::view() noexcept {
    return MatrixView{data_.data(), rows_, cols_};  // указатель на буфер и размеры
}

MatrixView Matrix::view() const noexcept {
    // Ядра не пишут во входные дескрипторы, поэтому снятие const здесь безопасно.
    return MatrixView{const_cast<value_type*>(data_.data()), rows_, cols_};
}

void Matrix::fill_sequence(value_type start_value) noexcept {
    auto next = static_cast<std::uint32_t>(start_value);  // беззнаковая арифметика: без UB при переносе
    for (auto& value : data_) {                           // по всем элементам
        value = static_cast<value_type>(next++);          // следующее значение (модульное преобразование)
    }
}

std::ostream& operator<<(std::ostream& os, const Matrix& m) {
    for (Matrix::size_type i = 0; i < m.rows(); ++i) {  // по строкам
        os << "[ ";                                     // начало строки
        for (Matrix::size_type j = 0; j < m.cols(); ++j) {  // по столбцам
            os << std::setw(6) << m(i, j) << ' ';       // элемент шириной 6
        }
        os << "]\n";                                    // конец строки
    }
    return os;                                          // для цепочек <<
}

void Matrix::print(std::string_view title, std::ostream& os) const {
    os << "--- " << title << " (" << rows_ << 'x' << cols_ << ") ---\n"  // заголовок с размерами
       << *this << '\n';                                                 // содержимое и пустая строка
}

void Matrix::print(std::string_view title) const {
    print(title, std::cout);  // вывод по умолчанию в stdout
}

std::expected<Matrix, AsmStatus> Matrix::add(const Matrix& a, const Matrix& b) {
    if (a.rows_ != b.rows_ || a.cols_ != b.cols_) {              // размеры должны совпадать
        return std::unexpected(AsmStatus::ERR_DIMENSION_MISMATCH);  // без лишней аллокации
    }
    Matrix result(a.rows_, a.cols_);                  // результат того же размера
    if (result.empty()) {                             // нечего считать (и нет указателей для asm)
        return result;                                // пустой результат корректен
    }
    const MatrixView va = a.view();                   // дескриптор A
    const MatrixView vb = b.view();                   // дескриптор B
    MatrixView vr = result.view();                    // дескриптор результата
    const AsmStatus status = asm_matrix_add(&va, &vb, &vr);  // вычисление в asm
    return finish(std::move(result), status);         // матрица или код ошибки
}

std::expected<Matrix, AsmStatus> Matrix::sub(const Matrix& a, const Matrix& b) {
    if (a.rows_ != b.rows_ || a.cols_ != b.cols_) {              // размеры должны совпадать
        return std::unexpected(AsmStatus::ERR_DIMENSION_MISMATCH);  // без лишней аллокации
    }
    Matrix result(a.rows_, a.cols_);                  // результат того же размера
    if (result.empty()) {                             // нечего считать
        return result;                                // пустой результат корректен
    }
    const MatrixView va = a.view();                   // дескриптор A
    const MatrixView vb = b.view();                   // дескриптор B
    MatrixView vr = result.view();                    // дескриптор результата
    const AsmStatus status = asm_matrix_sub(&va, &vb, &vr);  // вычисление в asm
    return finish(std::move(result), status);         // матрица или код ошибки
}

std::expected<Matrix, AsmStatus> Matrix::mul(const Matrix& a, const Matrix& b) {
    if (a.cols_ != b.rows_) {                                    // A.cols должно равняться B.rows
        return std::unexpected(AsmStatus::ERR_DIMENSION_MISMATCH);  // без лишней аллокации
    }
    Matrix result(a.rows_, b.cols_);                  // результат M x N, заполнен нулями
    if (result.empty() || a.cols_ == 0) {             // пустой результат или пустая сумма
        return result;                                // нули корректны без вызова asm
    }
    const MatrixView va = a.view();                   // дескриптор A
    const MatrixView vb = b.view();                   // дескриптор B
    MatrixView vr = result.view();                    // дескриптор результата (не пересекается с A, B)
    const AsmStatus status = asm_matrix_mul(&va, &vb, &vr);  // вычисление в asm
    return finish(std::move(result), status);         // матрица или код ошибки
}

std::expected<Matrix, AsmStatus> Matrix::transpose(const Matrix& a) {
    Matrix result(a.cols_, a.rows_);                  // результат N x M
    if (result.empty()) {                             // нечего транспонировать
        return result;                                // пустой результат корректен
    }
    const MatrixView va = a.view();                   // дескриптор A
    MatrixView vr = result.view();                    // дескриптор результата (отдельный буфер)
    const AsmStatus status = asm_matrix_transpose(&va, &vr);  // вычисление в asm
    return finish(std::move(result), status);         // матрица или код ошибки
}

Matrix Matrix::operator+(const Matrix& other) const {
    return unwrap(add(*this, other));        // сумма или invalid_argument
}

Matrix Matrix::operator-(const Matrix& other) const {
    return unwrap(sub(*this, other));        // разность или invalid_argument
}

Matrix Matrix::operator*(const Matrix& other) const {
    return unwrap(mul(*this, other));        // произведение или invalid_argument
}

Matrix Matrix::transposed() const {
    return unwrap(transpose(*this));         // транспонирование или исключение
}

}  // namespace ctrlwork::matrix
