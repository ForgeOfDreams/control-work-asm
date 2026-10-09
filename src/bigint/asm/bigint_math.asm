; =========================================================================================
; Aрифметика над беззнаковыми многобитными числами (модули, без знака)
;
; Число = массив 64-битных лимбов, младший лимб первым (little-endian по лимбам).
; Знак, нормализацию и выделение буферов выполняет C++ (bigint.cpp); ядра работают
; только с модулем и сами ничего не выделяют.
;
; Регистры: RAX RCX RDX R8-R11 свободны в обоих ABI; RBX RBP RSI RDI R12-R15 сохраняются
; (push/pop) - RSI/RDI в Win64 callee-saved, поэтому тоже сохраняются там, где используются.
; =========================================================================================
include 'ctrlwork_abi.inc'

ASM_SUCCESS                = 0
ASM_ERR_DIMENSION_MISMATCH = 1
ASM_ERR_NULL_POINTER       = 2
ASM_ERR_DIV_BY_ZERO        = 3
ASM_ERR_OVERFLOW           = 4
ASM_ERR_ALIASING           = 5

; struct BigIntView { uint64_t* data; uint64_t len; };  len >= 1
BV_DATA = 0
BV_LEN  = 8

; AsmStatus asm_bigint_add   (const BigIntView* a, const BigIntView* b, BigIntView* r)  r = a + b
; AsmStatus asm_bigint_sub   (const BigIntView* a, const BigIntView* b, BigIntView* r)  r = a - b (a >= b)
; AsmStatus asm_bigint_mul   (const BigIntView* a, const BigIntView* b, BigIntView* r)  r = a * b
; AsmStatus asm_bigint_divmod(const BigIntView* a, const BigIntView* b,
;                             BigIntView* q, BigIntView* r)                             q = a / b, r = a % b
public asm_bigint_add
public asm_bigint_sub
public asm_bigint_mul
public asm_bigint_divmod

cw_text

; -----------------------------------------------------------------------------------------
; r = a + b.  Требования: r.len >= max(a.len, b.len); r может совпадать с a или b целиком,
; но не перекрываться частично. Перенос из старшего лимба r -> ASM_ERR_OVERFLOW.
; RBX=A, R12=LA, RBP=B, R13=LB, R14=R, R15=RL, RCX=i, R8=перенос
; -----------------------------------------------------------------------------------------
asm_bigint_add:
    mov r10, arg1                           ; дескриптор A (копируем до использования arg-регистров)
    mov r11, arg2                           ; дескриптор B
    mov rax, arg3                           ; дескриптор R
    push rbx                                ; сохраняем callee-saved регистры
    push rbp
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; дескрипторы не нулевые
    jz .null
    test r11, r11
    jz .null
    test rax, rax
    jz .null
    mov rbx, [r10 + BV_DATA]                ; A.data
    mov r12, [r10 + BV_LEN]                 ; LA
    mov rbp, [r11 + BV_DATA]                ; B.data
    mov r13, [r11 + BV_LEN]                 ; LB
    mov r14, [rax + BV_DATA]                ; R.data
    mov r15, [rax + BV_LEN]                 ; RL
    test rbx, rbx                           ; буферы не нулевые
    jz .null
    test rbp, rbp
    jz .null
    test r14, r14
    jz .null
    test r12, r12                           ; длины >= 1
    jz .dim
    test r13, r13
    jz .dim
    cmp r15, r12                            ; RL >= LA
    jb .dim
    cmp r15, r13                            ; RL >= LB
    jb .dim
    xor ecx, ecx                            ; i = 0
    xor r8d, r8d                            ; перенос = 0
.loop:
    cmp rcx, r15                            ; все лимбы результата обработаны?
    jae .end
    xor eax, eax                            ; x = 0
    cmp rcx, r12                            ; i < LA ?
    jae .no_a
    mov rax, [rbx + rcx*8]                  ; x = A[i]
.no_a:
    xor r9d, r9d                            ; y = 0
    cmp rcx, r13                            ; i < LB ?
    jae .no_b
    mov r9, [rbp + rcx*8]                   ; y = B[i]
.no_b:
    xor edx, edx                            ; новый перенос = 0
    add rax, r9                             ; x + y
    adc rdx, 0                              ; перенос от x + y
    add rax, r8                             ; + старый перенос
    adc rdx, 0                              ; (суммарно перенос <= 1)
    mov [r14 + rcx*8], rax                  ; R[i]
    mov r8, rdx                             ; перенос в следующий лимб
    inc rcx                                 ; i++
    jmp .loop
.end:
    test r8, r8                             ; перенос за пределы R?
    jnz .overflow
.ok:
    xor eax, eax                            ; ASM_SUCCESS
    jmp .exit
.null:
    mov eax, ASM_ERR_NULL_POINTER
    jmp .exit
.dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    jmp .exit
.overflow:
    mov eax, ASM_ERR_OVERFLOW
.exit:
    pop r15                                 ; восстанавливаем регистры
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; -----------------------------------------------------------------------------------------
; r = a - b.  Те же требования, что у add. Если a < b (заём из старшего лимба) ->
; ASM_ERR_OVERFLOW (результат отрицателен).
; -----------------------------------------------------------------------------------------
asm_bigint_sub:
    mov r10, arg1                           ; дескриптор A
    mov r11, arg2                           ; дескриптор B
    mov rax, arg3                           ; дескриптор R
    push rbx                                ; сохраняем callee-saved регистры
    push rbp
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; дескрипторы не нулевые
    jz .null
    test r11, r11
    jz .null
    test rax, rax
    jz .null
    mov rbx, [r10 + BV_DATA]                ; A.data
    mov r12, [r10 + BV_LEN]                 ; LA
    mov rbp, [r11 + BV_DATA]                ; B.data
    mov r13, [r11 + BV_LEN]                 ; LB
    mov r14, [rax + BV_DATA]                ; R.data
    mov r15, [rax + BV_LEN]                 ; RL
    test rbx, rbx                           ; буферы не нулевые
    jz .null
    test rbp, rbp
    jz .null
    test r14, r14
    jz .null
    test r12, r12                           ; длины >= 1
    jz .dim
    test r13, r13
    jz .dim
    cmp r15, r12                            ; RL >= LA
    jb .dim
    cmp r15, r13                            ; RL >= LB
    jb .dim
    xor ecx, ecx                            ; i = 0
    xor r8d, r8d                            ; заём = 0
.loop:
    cmp rcx, r15                            ; все лимбы результата обработаны?
    jae .end
    xor eax, eax                            ; x = 0
    cmp rcx, r12                            ; i < LA ?
    jae .no_a
    mov rax, [rbx + rcx*8]                  ; x = A[i]
.no_a:
    xor r9d, r9d                            ; y = 0
    cmp rcx, r13                            ; i < LB ?
    jae .no_b
    mov r9, [rbp + rcx*8]                   ; y = B[i]
.no_b:
    xor edx, edx                            ; новый заём = 0
    sub rax, r9                             ; x - y
    adc rdx, 0                              ; заём от x - y
    sub rax, r8                             ; - старый заём
    adc rdx, 0                              ; (суммарно заём <= 1)
    mov [r14 + rcx*8], rax                  ; R[i]
    mov r8, rdx                             ; заём в следующий лимб
    inc rcx                                 ; i++
    jmp .loop
.end:
    test r8, r8                             ; остался заём -> a < b
    jnz .overflow
.ok:
    xor eax, eax                            ; ASM_SUCCESS
    jmp .exit
.null:
    mov eax, ASM_ERR_NULL_POINTER
    jmp .exit
.dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    jmp .exit
.overflow:
    mov eax, ASM_ERR_OVERFLOW
.exit:
    pop r15                                 ; восстанавливаем регистры
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; -----------------------------------------------------------------------------------------
; r = a * b (школьное умножение O(LA*LB)). Требования: r.len >= a.len + b.len; r не
; перекрывается с a и b (a и b могут совпадать - возведение в квадрат).
; RBX=A, R13=LA, RBP=B, R14=LB, R12=R, R15=RL затем i, R8=j, R9=перенос, R10=A[i], R11=i+j
; -----------------------------------------------------------------------------------------
asm_bigint_mul:
    mov r10, arg1                           ; дескриптор A
    mov r11, arg2                           ; дескриптор B
    mov rax, arg3                           ; дескриптор R
    push rbx                                ; сохраняем callee-saved регистры
    push rbp
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; дескрипторы не нулевые
    jz .null
    test r11, r11
    jz .null
    test rax, rax
    jz .null
    mov rbx, [r10 + BV_DATA]                ; A.data
    mov r13, [r10 + BV_LEN]                 ; LA
    mov rbp, [r11 + BV_DATA]                ; B.data
    mov r14, [r11 + BV_LEN]                 ; LB
    mov r12, [rax + BV_DATA]                ; R.data
    mov r15, [rax + BV_LEN]                 ; RL
    test rbx, rbx                           ; буферы не нулевые
    jz .null
    test rbp, rbp
    jz .null
    test r12, r12
    jz .null
    test r13, r13                           ; длины >= 1
    jz .dim
    test r14, r14
    jz .dim
    lea rcx, [r13 + r14]                    ; нужно LA + LB лимбов
    cmp r15, rcx
    jb .dim
    lea rax, [r12 + r15*8]                  ; конец R
    cmp rbx, rax                            ; A начинается после конца R?
    jae .a_ok
    lea rax, [rbx + r13*8]                  ; конец A
    cmp r12, rax                            ; R начинается раньше конца A -> перекрытие
    jb .alias
.a_ok:
    lea rax, [r12 + r15*8]                  ; конец R
    cmp rbp, rax                            ; B начинается после конца R?
    jae .b_ok
    lea rax, [rbp + r14*8]                  ; конец B
    cmp r12, rax                            ; перекрытие R и B
    jb .alias
.b_ok:
    xor ecx, ecx                            ; обнуляем R
.zero:
    cmp rcx, r15
    jae .zeroed
    mov qword [r12 + rcx*8], 0
    inc rcx
    jmp .zero
.zeroed:
    xor r15d, r15d                          ; i = 0 (RL больше не нужна)
.row:
    cmp r15, r13                            ; i < LA ?
    jae .ok
    mov r10, [rbx + r15*8]                  ; A[i]
    xor r8d, r8d                            ; j = 0
    xor r9d, r9d                            ; перенос = 0
.col:
    cmp r8, r14                             ; j < LB ?
    jae .row_end
    mov rax, r10                            ; rax = A[i]
    mul qword [rbp + r8*8]                  ; rdx:rax = A[i] * B[j]
    lea r11, [r15 + r8]                     ; k = i + j
    add rax, [r12 + r11*8]                  ; + R[k]
    adc rdx, 0
    add rax, r9                             ; + перенос
    adc rdx, 0                              ; (не переполняется: max = 2^128 - 1)
    mov [r12 + r11*8], rax                  ; R[k] = младшая часть
    mov r9, rdx                             ; перенос = старшая часть
    inc r8                                  ; j++
    jmp .col
.row_end:
    lea r11, [r15 + r14]                    ; k = i + LB
    mov [r12 + r11*8], r9                   ; старший лимб строки (там был 0)
    inc r15                                 ; i++
    jmp .row
.ok:
    xor eax, eax                            ; ASM_SUCCESS
    jmp .exit
.null:
    mov eax, ASM_ERR_NULL_POINTER
    jmp .exit
.dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    jmp .exit
.alias:
    mov eax, ASM_ERR_ALIASING
.exit:
    pop r15                                 ; восстанавливаем регистры
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; -----------------------------------------------------------------------------------------
; q = a / b, r = a % b (беззнаково, усечение). Требования: q.len >= a.len, r.len >= b.len;
; q, r не перекрываются между собой и с a, b. q и r заполняются нулями целиком.
; b == 0 -> ASM_ERR_DIV_BY_ZERO (выходные буферы не изменены).
; Делитель из одного лимба: аппаратный div, O(n). Иначе - двоичное деление "сдвиг-вычитание":
; на каждый бит делимого R = R*2 + бит; если R >= B, то R -= B и бит частного = 1.
; RBX=A, R12=la, RBP=B, R13=lb, R14=Q, R15=QL/счётчик битов, RSI=R, RDI=RL
; -----------------------------------------------------------------------------------------
asm_bigint_divmod:
    mov r10, arg1                           ; дескриптор A
    mov r11, arg2                           ; дескриптор B
    mov rax, arg3                           ; дескриптор Q
    mov r8, arg4                            ; дескриптор R
    push rbx                                ; сохраняем callee-saved регистры
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; дескрипторы не нулевые
    jz .null
    test r11, r11
    jz .null
    test rax, rax
    jz .null
    test r8, r8
    jz .null
    mov rbx, [r10 + BV_DATA]                ; A.data
    mov r12, [r10 + BV_LEN]                 ; LA
    mov rbp, [r11 + BV_DATA]                ; B.data
    mov r13, [r11 + BV_LEN]                 ; LB
    mov r14, [rax + BV_DATA]                ; Q.data
    mov r15, [rax + BV_LEN]                 ; QL
    mov rsi, [r8 + BV_DATA]                 ; R.data
    mov rdi, [r8 + BV_LEN]                  ; RL
    test rbx, rbx                           ; буферы не нулевые
    jz .null
    test rbp, rbp
    jz .null
    test r14, r14
    jz .null
    test rsi, rsi
    jz .null
    test r12, r12                           ; длины >= 1
    jz .dim
    test r13, r13
    jz .dim
    test r15, r15
    jz .dim
    test rdi, rdi
    jz .dim
    cmp r15, r12                            ; QL >= LA
    jb .dim
    cmp rdi, r13                            ; RL >= LB
    jb .dim
    lea rax, [r14 + r15*8]                  ; --- перекрытие A и Q
    cmp rbx, rax
    jae .o1
    lea rax, [rbx + r12*8]
    cmp r14, rax
    jb .alias
.o1:
    lea rax, [rsi + rdi*8]                  ; --- перекрытие A и R
    cmp rbx, rax
    jae .o2
    lea rax, [rbx + r12*8]
    cmp rsi, rax
    jb .alias
.o2:
    lea rax, [r14 + r15*8]                  ; --- перекрытие B и Q
    cmp rbp, rax
    jae .o3
    lea rax, [rbp + r13*8]
    cmp r14, rax
    jb .alias
.o3:
    lea rax, [rsi + rdi*8]                  ; --- перекрытие B и R
    cmp rbp, rax
    jae .o4
    lea rax, [rbp + r13*8]
    cmp rsi, rax
    jb .alias
.o4:
    lea rax, [rsi + rdi*8]                  ; --- перекрытие Q и R
    cmp r14, rax
    jae .o5
    lea rax, [r14 + r15*8]
    cmp rsi, rax
    jb .alias
.o5:
    mov rcx, r13                            ; отбрасываем старшие нулевые лимбы B
.trim_b:
    test rcx, rcx                           ; B == 0 ?
    jz .div0
    cmp qword [rbp + rcx*8 - 8], 0
    jne .b_trimmed
    dec rcx
    jmp .trim_b
.b_trimmed:
    mov r13, rcx                            ; lb >= 1
    mov rcx, r12                            ; отбрасываем старшие нулевые лимбы A
.trim_a:
    test rcx, rcx
    jz .a_trimmed
    cmp qword [rbx + rcx*8 - 8], 0
    jne .a_trimmed
    dec rcx
    jmp .trim_a
.a_trimmed:
    mov r12, rcx                            ; la (может быть 0)
    xor ecx, ecx                            ; обнуляем Q
.zq:
    cmp rcx, r15
    jae .zq_done
    mov qword [r14 + rcx*8], 0
    inc rcx
    jmp .zq
.zq_done:
    xor ecx, ecx                            ; обнуляем R
.zr:
    cmp rcx, rdi
    jae .zr_done
    mov qword [rsi + rcx*8], 0
    inc rcx
    jmp .zr
.zr_done:
    cmp r13, 1                              ; делитель из одного лимба?
    jne .general
    mov rbp, [rbp]                          ; d = B[0] (указатель B больше не нужен)
    xor edx, edx                            ; остаток = 0
    mov rcx, r12                            ; i = la
.small:
    test rcx, rcx
    jz .small_done
    dec rcx                                 ; i--
    mov rax, [rbx + rcx*8]                  ; rdx:rax = остаток:A[i]
    div rbp                                 ; rax = частное, rdx = новый остаток
    mov [r14 + rcx*8], rax                  ; Q[i]
    jmp .small
.small_done:
    mov [rsi], rdx                          ; R[0] = остаток
    jmp .ok
.general:
    mov r15, r12                            ; число бит делимого = la * 64
    shl r15, 6
.bit_loop:
    test r15, r15                           ; все биты обработаны?
    jz .ok
    dec r15                                 ; t = индекс текущего бита (от старшего к младшему)
    mov rdx, r15
    shr rdx, 6                              ; номер лимба
    mov rax, [rbx + rdx*8]
    mov rcx, r15                            ; cl = t mod 64
    shr rax, cl
    and eax, 1                              ; rax = бит t делимого
    mov r8, r13                             ; счётчик = lb
    xor ecx, ecx                            ; индекс = 0 (xor сбрасывает CF - ставим CF после него)
    neg rax                                 ; CF = бит
.shl:
    rcl qword [rsi + rcx*8], 1              ; R = R*2 + CF; CF = вытолкнутый бит
    inc rcx                                 ; inc/dec не меняют CF
    dec r8
    jnz .shl
    sbb r9, r9                              ; r9 = -1, если бит вытолкнут за старший лимб
    test r9, r9
    jnz .do_sub                             ; R >= 2^(64*lb) > B -> вычитаем
    mov rcx, r13                            ; сравнение R с B со старшего лимба
.cmp_loop:
    dec rcx
    mov rax, [rsi + rcx*8]
    cmp rax, [rbp + rcx*8]
    jne .cmp_decided
    test rcx, rcx
    jnz .cmp_loop
    jmp .do_sub                             ; R == B -> вычитаем
.cmp_decided:
    jb .bit_loop                            ; R < B -> бит частного 0
.do_sub:
    xor ecx, ecx                            ; индекс = 0, CF = 0
    mov r8, r13                             ; счётчик = lb
.sub_loop:
    mov rax, [rbp + rcx*8]
    sbb [rsi + rcx*8], rax                  ; R -= B с заёмом
    inc rcx
    dec r8
    jnz .sub_loop
    mov rcx, r15                            ; Q |= 1 << t
    mov rdx, 1
    shl rdx, cl
    mov rcx, r15
    shr rcx, 6
    or [r14 + rcx*8], rdx
    jmp .bit_loop
.ok:
    xor eax, eax                            ; ASM_SUCCESS
    jmp .exit
.null:
    mov eax, ASM_ERR_NULL_POINTER
    jmp .exit
.dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    jmp .exit
.div0:
    mov eax, ASM_ERR_DIV_BY_ZERO
    jmp .exit
.alias:
    mov eax, ASM_ERR_ALIASING
.exit:
    pop r15                                 ; восстанавливаем регистры
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret
