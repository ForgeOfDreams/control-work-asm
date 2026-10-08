ctrlwork_add_module(matrix
    ALIAS Matrix
    SOURCES
        matrix.cpp
    ASM_SOURCES
        asm/matrix_math.asm
    INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include"
)
