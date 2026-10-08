; System V AMD64 (Linux/BSD, ELF64 object). Included first by every FASM source.
; Calling convention: args RDI, RSI, RDX, RCX, R8, R9 (+ stack), no shadow space,
; callee-saved: RBX RBP R12-R15.
format ELF64

CW_WIN64        = 0
CW_SHADOW_SPACE = 0

arg1 equ rdi
arg2 equ rsi
arg3 equ rdx
arg4 equ rcx

macro cw_text   { section '.text' executable }
macro cw_data   { section '.data' writeable }
macro cw_rodata { section '.rodata' }

; Empty marker section: tells the linker the stack does not need to be executable.
section '.note.GNU-stack'
