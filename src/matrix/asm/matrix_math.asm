; =========================================================================================
; Ядро матричных вычислений (int32, row-major, без stride)
;
; Собирается под Windows x64 (MS64 COFF) и Linux/BSD (ELF64, System V):
; формат файла, секции и регистры аргументов приходят из ctrlwork_abi.inc.
; Из регистров используются только те, что:
;   - свободны для функции в обоих ABI: RAX RCX RDX R8-R11
;   - сохраняются вызываемой функцией в обоих ABI: RBX R12-R15
; RSI/RDI не трогаем: в Win64 они callee-saved.
; =========================================================================================
include 'ctrlwork_abi.inc'

; Коды возврата = C++ enum class AsmStatus (должен иметь 32-битный базовый тип)
ASM_SUCCESS                 = 0
ASM_ERR_DIMENSION_MISMATCH  = 1
ASM_ERR_NULL_POINTER        = 2

; Раскладка MatrixView
;   struct MatrixView { int32_t* data; uint64_t rows; uint64_t cols; };
MV_DATA = 0
MV_ROWS = 8
MV_COLS = 16

; Аргументы: arg1, arg2, arg3 (см. ctrlwork_abi.inc)
; AsmStatus asm_matrix_add      (const MatrixView* a, const MatrixView* b, MatrixView* c)
; AsmStatus asm_matrix_sub      (const MatrixView* a, const MatrixView* b, MatrixView* c)
; AsmStatus asm_matrix_mul      (const MatrixView* a, const MatrixView* b, MatrixView* c)
; AsmStatus asm_matrix_transpose(const MatrixView* a, MatrixView* b)
public asm_matrix_add
public asm_matrix_sub
public asm_matrix_mul
public asm_matrix_transpose

cw_text

; -----------------------------------------------------------------------------------------
; Общий пролог для add/sub: копирует аргументы, проверяет указатели и размеры c = a (op) b.
; На выходе: R8 = A.data, R9 = B.data, RDX = C.data, RCX = число элементов, RAX = 0 (индекс).
; Ошибки уходят на общие метки в конце файла; стек на этот момент не изменён.
; -----------------------------------------------------------------------------------------
macro MATRIX_ELEMENTWISE_PROLOGUE {
    mov r10, arg1                               ; A (копируем сразу: в Win64 arg-регистры = временные)
    mov r11, arg2                               ; B
    mov rax, arg3                               ; C
    test r10, r10
    jz matrix_err_null
    test r11, r11
    jz matrix_err_null
    test rax, rax
    jz matrix_err_null

    mov r8, [r10 + MV_DATA]                     ; A.data
    test r8, r8
    jz matrix_err_null
    mov r9, [r11 + MV_DATA]                     ; B.data
    test r9, r9
    jz matrix_err_null
    mov rdx, [rax + MV_DATA]                    ; C.data
    test rdx, rdx
    jz matrix_err_null

    mov rcx, [r10 + MV_ROWS]                    ; rows(A) == rows(B) == rows(C)
    cmp rcx, [r11 + MV_ROWS]
    jne matrix_err_dim
    cmp rcx, [rax + MV_ROWS]
    jne matrix_err_dim
    mov rcx, [r10 + MV_COLS]                    ; cols(A) == cols(B) == cols(C)
    cmp rcx, [r11 + MV_COLS]
    jne matrix_err_dim
    cmp rcx, [rax + MV_COLS]
    jne matrix_err_dim

    mov rcx, [r10 + MV_ROWS]
    imul rcx, [r10 + MV_COLS]                   ; RCX = rows * cols
    xor eax, eax                                ; RAX = i = 0
}

; -----------------------------------------------------------------------------------------
; C = A + B
; -----------------------------------------------------------------------------------------
asm_matrix_add:
    MATRIX_ELEMENTWISE_PROLOGUE
    test rcx, rcx
    jz matrix_ok                                ; пустая матрица - успех
.loop:
    mov r10d, dword [r8 + rax*4]                ; A[i]
    add r10d, dword [r9 + rax*4]                ; + B[i] (знаковое переполнение: wrap-around)
    mov dword [rdx + rax*4], r10d               ; C[i]
    inc rax
    cmp rax, rcx
    jb .loop
    jmp matrix_ok

; -----------------------------------------------------------------------------------------
; C = A - B
; -----------------------------------------------------------------------------------------
asm_matrix_sub:
    MATRIX_ELEMENTWISE_PROLOGUE
    test rcx, rcx
    jz matrix_ok
.loop:
    mov r10d, dword [r8 + rax*4]                ; A[i]
    sub r10d, dword [r9 + rax*4]                ; - B[i]
    mov dword [rdx + rax*4], r10d               ; C[i]
    inc rax
    cmp rax, rcx
    jb .loop
    jmp matrix_ok

; -----------------------------------------------------------------------------------------
; C = A * B   (A: M x K, B: K x N, C: M x N)
; C не должна перекрываться с A и B (проверяет вызывающий C++ код).
; -----------------------------------------------------------------------------------------
asm_matrix_mul:
    mov r10, arg1                               ; A
    mov r11, arg2                               ; B
    mov rax, arg3                               ; C
    test r10, r10
    jz matrix_err_null
    test r11, r11
    jz matrix_err_null
    test rax, rax
    jz matrix_err_null

    mov r8, [r10 + MV_DATA]                     ; A.data
    test r8, r8
    jz matrix_err_null
    mov r9, [r11 + MV_DATA]                     ; B.data
    test r9, r9
    jz matrix_err_null
    mov rdx, [rax + MV_DATA]                    ; C.data (остаётся в RDX до конца)
    test rdx, rdx
    jz matrix_err_null

    mov rcx, [r10 + MV_COLS]                    ; A.cols == B.rows
    cmp rcx, [r11 + MV_ROWS]
    jne matrix_err_dim
    mov rcx, [r10 + MV_ROWS]                    ; C.rows == A.rows
    cmp rcx, [rax + MV_ROWS]
    jne matrix_err_dim
    mov rcx, [r11 + MV_COLS]                    ; C.cols == B.cols
    cmp rcx, [rax + MV_COLS]
    jne matrix_err_dim

    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, [r10 + MV_COLS]                    ; R12 = K
    mov r13, [r11 + MV_COLS]                    ; R13 = N
    mov r11, [r10 + MV_ROWS]                    ; R11 = M
                                                ; RAX, R10 теперь свободные временные

    xor r14d, r14d                              ; i = 0
.loop_i:
    cmp r14, r11
    jae .done
    xor r15d, r15d                              ; j = 0
.loop_j:
    cmp r15, r13
    jae .next_i
    xor ebx, ebx                                ; sum = 0
    xor ecx, ecx                                ; k = 0
.loop_k:
    cmp rcx, r12
    jae .store
    mov rax, r14                                ; A[i][k] = A.data[i*K + k]
    imul rax, r12
    add rax, rcx
    mov eax, dword [r8 + rax*4]
    mov r10, rcx                                ; B[k][j] = B.data[k*N + j]
    imul r10, r13
    add r10, r15
    mov r10d, dword [r9 + r10*4]
    imul eax, r10d
    add ebx, eax
    inc rcx
    jmp .loop_k
.store:
    mov rax, r14                                ; C[i][j] = C.data[i*N + j]
    imul rax, r13
    add rax, r15
    mov dword [rdx + rax*4], ebx
    inc r15
    jmp .loop_j
.next_i:
    inc r14
    jmp .loop_i
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    jmp matrix_ok

; -----------------------------------------------------------------------------------------
; B = A^T   (A: R x C, B: C x R). B не должна перекрываться с A.
; Чтение A идёт подряд, запись в B - с шагом R*4 байт (без умножений в цикле).
; -----------------------------------------------------------------------------------------
asm_matrix_transpose:
    mov r10, arg1                               ; A
    mov r11, arg2                               ; B
    test r10, r10
    jz matrix_err_null
    test r11, r11
    jz matrix_err_null
    mov r8, [r10 + MV_DATA]                     ; R8 = указатель чтения A (двигается на +4)
    test r8, r8
    jz matrix_err_null
    mov r9, [r11 + MV_DATA]                     ; R9 = B.data + i*4 (начало записи для строки i)
    test r9, r9
    jz matrix_err_null

    mov rax, [r10 + MV_ROWS]                    ; A.rows == B.cols
    cmp rax, [r11 + MV_COLS]
    jne matrix_err_dim
    mov rcx, [r10 + MV_COLS]                    ; A.cols == B.rows
    cmp rcx, [r11 + MV_ROWS]
    jne matrix_err_dim

    test rax, rax
    jz matrix_ok
    test rcx, rcx
    jz matrix_ok

    push rbx
    lea r10, [rax*4]                            ; R10 = шаг записи в байтах = A.rows * 4
    mov r11, rcx                                ; R11 = A.cols
                                                ; RAX = осталось строк, RCX = осталось столбцов
.row:
    mov rdx, r9                                 ; RDX = указатель записи B[0][i]
    mov rcx, r11
.col:
    mov ebx, dword [r8]                         ; A[i][j]
    mov dword [rdx], ebx                        ; -> B[j][i]
    add r8, 4
    add rdx, r10
    dec rcx
    jnz .col
    add r9, 4                                   ; следующая строка i -> следующий столбец B
    dec rax
    jnz .row
    pop rbx
    jmp matrix_ok

; -----------------------------------------------------------------------------------------
; Общие точки выхода (не public).
; -----------------------------------------------------------------------------------------
matrix_ok:
    xor eax, eax                                ; ASM_SUCCESS
    ret
matrix_err_null:
    mov eax, ASM_ERR_NULL_POINTER
    ret
matrix_err_dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    ret
