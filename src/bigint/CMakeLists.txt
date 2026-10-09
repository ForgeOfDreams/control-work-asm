ctrlwork_add_module(bigint
    ALIAS BigInt
    SOURCES
        bigint.cpp
    ASM_SOURCES
        asm/bigint_math.asm
        asm/bigint_io.asm
    INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include"
)
