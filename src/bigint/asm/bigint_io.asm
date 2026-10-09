; =========================================================================================
; ввод/вывод многобитных чисел: десятичная ASCII-строка <-> двоичное число
;
; Работает с модулем числа (знак обрабатывает C++). Число = массив 64-битных лимбов,
; младший лимб первым. Буферы выделяет вызывающий код.
; =========================================================================================
include 'ctrlwork_abi.inc'

ASM_SUCCESS                = 0
ASM_ERR_DIMENSION_MISMATCH = 1
ASM_ERR_NULL_POINTER       = 2
ASM_ERR_OVERFLOW           = 4
ASM_ERR_INVALID_FORMAT     = 6

BV_DATA = 0                                 ; struct BigIntView { uint64_t* data; uint64_t len; }
BV_LEN  = 8

TEN19 = 10000000000000000000                ; 10^19 - наибольшая степень 10, помещающаяся в 64 бита

; AsmStatus asm_bigint_from_decimal(const char* digits, uint64_t length, BigIntView* result)
; AsmStatus asm_bigint_to_decimal  (BigIntView* value, char* buffer, uint64_t capacity,
;                                   uint64_t* out_length)
public asm_bigint_from_decimal
public asm_bigint_to_decimal

cw_text

; -----------------------------------------------------------------------------------------
; Десятичная строка (только цифры '0'..'9', без знака и пробелов) -> число.
; Строка читается порциями до 19 цифр: result = result * 10^k + порция.
; Требования: length >= 1; result.len >= ceil(length / 19) (этого всегда достаточно).
; Ошибки: пустая строка или нецифра -> ASM_ERR_INVALID_FORMAT; не хватило лимбов ->
; ASM_ERR_OVERFLOW (содержимое result при ошибке не определено).
; RBX=указатель на цифры, R14=осталось цифр, R12=result.data, R13=result.len, R15=занято лимбов
; -----------------------------------------------------------------------------------------
asm_bigint_from_decimal:
    mov r10, arg1                           ; указатель на строку
    mov r11, arg2                           ; длина строки
    mov rax, arg3                           ; дескриптор результата
    push rbx                                ; сохраняем callee-saved регистры
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; строка и дескриптор не нулевые
    jz .null
    test rax, rax
    jz .null
    mov r12, [rax + BV_DATA]                ; result.data
    mov r13, [rax + BV_LEN]                 ; result.len
    test r12, r12
    jz .null
    test r13, r13                           ; result.len >= 1
    jz .dim
    test r11, r11                           ; пустая строка недопустима
    jz .format
    xor ecx, ecx                            ; обнуляем result
.zero:
    cmp rcx, r13
    jae .zeroed
    mov qword [r12 + rcx*8], 0
    inc rcx
    jmp .zero
.zeroed:
    mov rbx, r10                            ; указатель на текущую цифру
    mov r14, r11                            ; осталось цифр
    xor r15d, r15d                          ; занято лимбов = 0
.chunk:
    test r14, r14                           ; строка закончилась?
    jz .ok
    mov rcx, r14                            ; k = min(19, осталось)
    cmp rcx, 19
    jbe .k_ok
    mov ecx, 19
.k_ok:
    sub r14, rcx                            ; забираем k цифр
    xor r8d, r8d                            ; v = 0 (значение порции)
    mov r9d, 1                              ; m = 1 (будет 10^k)
.digit:
    movzx eax, byte [rbx]                   ; очередной символ
    sub eax, '0'
    cmp eax, 9                              ; беззнаково: ловит и символы < '0'
    ja .format
    imul r8, r8, 10                         ; v = v*10 + цифра (v < 10^19 < 2^64)
    add r8, rax
    imul r9, r9, 10                         ; m *= 10
    inc rbx
    dec rcx
    jnz .digit
    xor ecx, ecx                            ; i = 0; перенос = v (в R8)
.mulloop:
    cmp rcx, r15                            ; все занятые лимбы обработаны?
    jae .mul_done
    mov rax, [r12 + rcx*8]
    mul r9                                  ; rdx:rax = лимб * m
    add rax, r8                             ; + перенос
    adc rdx, 0
    mov [r12 + rcx*8], rax
    mov r8, rdx                             ; новый перенос
    inc rcx
    jmp .mulloop
.mul_done:
    test r8, r8                             ; остался перенос -> нужен новый лимб
    jz .chunk
    cmp r15, r13                            ; есть место?
    jae .overflow
    mov [r12 + r15*8], r8                   ; дописываем старший лимб
    inc r15
    jmp .chunk
.ok:
    xor eax, eax                            ; ASM_SUCCESS
    jmp .exit
.null:
    mov eax, ASM_ERR_NULL_POINTER
    jmp .exit
.dim:
    mov eax, ASM_ERR_DIMENSION_MISMATCH
    jmp .exit
.format:
    mov eax, ASM_ERR_INVALID_FORMAT
    jmp .exit
.overflow:
    mov eax, ASM_ERR_OVERFLOW
.exit:
    pop r15                                 ; восстанавливаем регистры
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; -----------------------------------------------------------------------------------------
; Число -> десятичная строка (без знака, без ведущих нулей, без завершающего нуля).
; ВНИМАНИЕ: value разрушается (делится на 10^19 до нуля) - передавайте рабочую копию.
; Цифры записываются в buffer[0 .. *out_length). Требование: capacity >= числа цифр;
; достаточно 20 * value.len. Иначе -> ASM_ERR_OVERFLOW.
; Метод: делим на 10^19, остаток (порция из 19 цифр) раскладываем делением на 10 и пишем
; цифры с конца буфера; в конце сдвигаем результат в начало.
; RBX=value.data, R12=n (значащих лимбов), R13=buffer, R14=позиция записи (с конца),
; R15=capacity, RSI=out_length*, RBP=10^19
; -----------------------------------------------------------------------------------------
asm_bigint_to_decimal:
    mov r10, arg1                           ; дескриптор числа
    mov r11, arg2                           ; буфер
    mov rax, arg3                           ; capacity
    mov r8, arg4                            ; указатель на out_length
    push rbx                                ; сохраняем callee-saved регистры
    push rbp
    push rsi
    push r12
    push r13
    push r14
    push r15
    test r10, r10                           ; указатели не нулевые
    jz .null
    test r11, r11
    jz .null
    test r8, r8
    jz .null
    mov rbx, [r10 + BV_DATA]                ; value.data
    mov r12, [r10 + BV_LEN]                 ; value.len
    test rbx, rbx
    jz .null
    test r12, r12                           ; value.len >= 1
    jz .dim
    test rax, rax                           ; нужна хотя бы одна позиция
    jz .overflow
    mov r13, r11                            ; buffer
    mov r15, rax                            ; capacity
    mov r14, rax                            ; позиция записи = capacity
    mov rsi, r8                             ; out_length*
    mov rbp, TEN19                          ; делитель 10^19
.trim:
    test r12, r12                           ; отбрасываем старшие нулевые лимбы
    jz .trimmed
    cmp qword [rbx + r12*8 - 8], 0
    jne .trimmed
    dec r12
    jmp .trim
.trimmed:
    test r12, r12                           ; число равно нулю?
    jnz .loop
    dec r14                                 ; ноль -> единственная цифра '0'
    mov byte [r13 + r14], '0'
    jmp .moved
.loop:
    xor edx, edx                            ; остаток = 0
    mov rcx, r12                            ; i = n
.div:
    dec rcx                                 ; i--
    mov rax, [rbx + rcx*8]                  ; rdx:rax = остаток:value[i]
    div rbp                                 ; делим на 10^19
    mov [rbx + rcx*8], rax                  ; value[i] = частное
    test rcx, rcx
    jnz .div
    mov r9, rdx                             ; порция (остаток) = 0 .. 10^19-1
.trim2:
    test r12, r12                           ; отбрасываем нулевые старшие лимбы частного
    jz .trimmed2
    cmp qword [rbx + r12*8 - 8], 0
    jne .trimmed2
    dec r12
    jmp .trim2
.trimmed2:
    mov rax, r9                             ; раскладываем порцию на цифры
    mov r10d, 10
    test r12, r12                           ; остались ещё порции?
    jz .last
    mov r11d, 19                            ; не последняя порция: ровно 19 цифр (с нулями)
.fixed:
    test r14, r14                           ; место в буфере есть?
    jz .overflow
    xor edx, edx
    div r10                                 ; rax = rax / 10, rdx = цифра
    add dl, '0'
    dec r14
    mov [r13 + r14], dl
    dec r11
    jnz .fixed
    jmp .loop
.last:
    test r14, r14                           ; последняя (старшая) порция: без ведущих нулей
    jz .overflow
    xor edx, edx
    div r10
    add dl, '0'
    dec r14
    mov [r13 + r14], dl
    test rax, rax
    jnz .last
.moved:
    mov rcx, r15                            ; длина = capacity - позиция
    sub rcx, r14
    mov [rsi], rcx                          ; *out_length
    xor eax, eax                            ; k = 0
.move:
    cmp rax, rcx                            ; сдвигаем цифры в начало буфера
    jae .ok
    lea rdx, [r14 + rax]
    movzx r8d, byte [r13 + rdx]
    mov [r13 + rax], r8b
    inc rax
    jmp .move
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
    pop rsi
    pop rbp
    pop rbx
    ret
