; Windows x64 (MS64 COFF object). Included first by every FASM source.
; Calling convention: args RCX, RDX, R8, R9 (+ stack), 32 bytes shadow space,
; callee-saved: RBX RBP RDI RSI R12-R15, XMM6-XMM15.
format MS64 COFF

CW_WIN64        = 1
CW_SHADOW_SPACE = 32

arg1 equ rcx
arg2 equ rdx
arg3 equ r8
arg4 equ r9

macro cw_text   { section '.text'  code readable executable }
macro cw_data   { section '.data'  data readable writeable }
macro cw_rodata { section '.rdata' data readable }
