include_guard(GLOBAL)

# Directory with per-ABI prologues: /win64/ctrlwork_abi.inc, /elf64/ctrlwork_abi.inc
set(CTRLWORK_ASM_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}/../../src/asm"
    CACHE INTERNAL "FASM common include root")

# ctrlwork_target_add_fasm(<target>
#     SOURCES <file.asm>...        # relative to the current source dir
#     [DEPENDS <files>...])        # extra .inc files that should trigger a rebuild
#
# FASM is not a CMake language, so every .asm is assembled by a custom command
# into an object file that is linked into <target> as an external object.
# The ABI (Win64 COFF vs. System V ELF64) is selected by pointing FASM's INCLUDE
# path at the matching directory, so sources just do: include 'ctrlwork_abi.inc'
function(ctrlwork_target_add_fasm target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;DEPENDS")

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "ctrlwork_target_add_fasm: '${target}' is not a target")
    endif()
    if(NOT ARG_SOURCES)
        return()
    endif()

    # platform checks
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        message(FATAL_ERROR
            "FASM sources are x86-64 only (CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}').")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_abi win64)
        set(_ext ".obj")
    elseif(CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD|NetBSD|OpenBSD)$")
        set(_abi elf64)
        set(_ext ".o")
    else()
        message(FATAL_ERROR
            "FASM sources are not supported on '${CMAKE_SYSTEM_NAME}' (only Windows and ELF systems).")
    endif()

    # assembler
    if(NOT CTRLWORK_FASM)
        find_program(CTRLWORK_FASM NAMES fasm fasm.x64 fasm.exe
            DOC "Flat assembler (FASM) executable")
    endif()
    if(NOT CTRLWORK_FASM)
        message(FATAL_ERROR
            "FASM not found. Install it or pass -DCTRLWORK_FASM=/path/to/fasm")
    endif()

    set(_abi_dir "${CTRLWORK_ASM_COMMON_DIR}/${_abi}")
    set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/fasm/${target}")
    file(MAKE_DIRECTORY "${_out_dir}")

    # one custom command per source
    set(_objs "")
    foreach(_src IN LISTS ARG_SOURCES)
        cmake_path(ABSOLUTE_PATH _src
            NORMALIZE OUTPUT_VARIABLE _abs)
        cmake_path(GET _abs STEM _stem)
        set(_obj "${_out_dir}/${_stem}${_ext}")

        if(_obj IN_LIST _objs)
            message(FATAL_ERROR
                "ctrlwork_target_add_fasm(${target}): two sources share the name '${_stem}'")
        endif()

        add_custom_command(
            OUTPUT  "${_obj}"
            COMMAND "${CMAKE_COMMAND}" -E env "INCLUDE=${_abi_dir}"
                    "${CTRLWORK_FASM}" "${_abs}" "${_obj}"
            DEPENDS "${_abs}" "${_abi_dir}/ctrlwork_abi.inc" ${ARG_DEPENDS}
            COMMENT "FASM ${_src}"
            VERBATIM)
        list(APPEND _objs "${_obj}")
    endforeach()

    set_source_files_properties(${_objs} PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED       TRUE)
    target_sources(${target} PRIVATE ${_objs})
endfunction()

# ctrlwork_target_export_asm_symbols(<target> <symbol>...)
#
# Makes functions that are DEFINED IN ASM part of the public ABI of a SHARED
# library. Needed on Windows only: a COFF `public` symbol is visible to the
# linker but is not put into the DLL export table, and __declspec(dllexport)
# cannot be attached to code that lives in an .asm file, so we generate a .def.
# ELF: global symbols of a shared object are exported already -> nothing to do.
# Static libraries need nothing either. A .def is combined by the linker with
# any __declspec(dllexport) coming from the C++ objects.
function(ctrlwork_target_export_asm_symbols target)
    if(NOT WIN32 OR NOT ARGN)
        return()
    endif()
    get_target_property(_type "${target}" TYPE)
    if(NOT _type STREQUAL "SHARED_LIBRARY")
        return()
    endif()

    string(JOIN "\n    " _body ${ARGN})
    set(_def "${CMAKE_CURRENT_BINARY_DIR}/${target}_asm_exports.def")
    file(GENERATE OUTPUT "${_def}" CONTENT "EXPORTS\n    ${_body}\n")
    target_sources(${target} PRIVATE "${_def}")
endfunction()
