# ControlWork: FASM & C++23 Math Core

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![CMake 3.30+](https://img.shields.io/badge/CMake-3.30%2B-064F8C.svg?logo=cmake)](https://cmake.org/)
[![Compiler Clang](https://img.shields.io/badge/Compiler-Clang-green.svg)](https://clang.llvm.org/)
[![Assembler FASM](https://img.shields.io/badge/Assembler-FASM-orange.svg)](https://flatassembler.net/)

This project implements low-level computational algorithms. The core math execution is written in **FASM** assembly, while high-level memory management, error diagnostics, and interfaces are implemented in **C++23** using the **Clang** compiler.

The build process is managed by **CMake**.

## Project Structure

The project is divided into 4 isolated modules:
1. `matrix` - Matrix operations (addition, subtraction, multiplication, transposition).
2. `bigint` - Multi-precision binary integer arithmetic (BigInt).
3. `bcd_packed` - Packed Binary-Coded Decimal (BCD) calculations.
4. `bcd_unpacked` - Unpacked BCD calculations.

* `include/` - C++ header files.
* `src/` - C++ source files (`.cpp`) and assembly code (`.asm`), compiled into a single static library `cw_core`.
* `examples/` - Executable files demonstrating the functionality of each module.

## Prerequisites

The following tools must be installed:
* **CMake** 3.30 or higher.
* **Clang** compiler (with C++23 support).
* **FASM** (Flat Assembler) accessible in system `PATH` (invoked via `fasm`).

## Build Instructions

To configure and build the project using CMake, run:

```bash
cmake --build build
```
