include_guard(GLOBAL)

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CtrlworkTargetDefaults)
include(CtrlworkFasm)

# ctrlwork_add_module(<name>
#     ALIAS        <ExportName>          # -> <Package>::<ExportName>
#     [SOURCES      <files>...]          # C++ sources; none => header-only (INTERFACE)
#     [ASM_SOURCES  <files>...]          # FASM sources (need SOURCES too)
#     [ASM_DEPENDS  <files>...]          # extra .inc files for the FASM sources
#     [ASM_EXPORTS  <symbols>...]        # asm functions that are public API of a DLL
#     [INCLUDE_DIR  <dir>]               # public headers, default: <module>/include
#     [PUBLIC_DEPS  <targets>...]
#     [PRIVATE_DEPS <targets>...])
#
# All paths are relative to the module's directory.
#
# Creates:   real target   ctrlwork_<name>
#            alias         ${CTRLWORK_PACKAGE_NAME}::<ExportName>
#            export header <ctrlwork/<name>/export.h>, macro CTRLWORK_<NAME>_EXPORT
# and, if CTRLWORK_INSTALL is ON, the install rules + export-set membership.
function(ctrlwork_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "ALIAS;INCLUDE_DIR" "SOURCES;ASM_SOURCES;ASM_DEPENDS;ASM_EXPORTS;PUBLIC_DEPS;PRIVATE_DEPS")

    if(NOT ARG_ALIAS)
        message(FATAL_ERROR "ctrlwork_add_module(${name}): ALIAS is required")
    endif()

    set(tgt "ctrlwork_${name}")
    string(TOUPPER "${name}" upper)
    set(base    "CTRLWORK_${upper}")
    if(ARG_INCLUDE_DIR)
        cmake_path(ABSOLUTE_PATH ARG_INCLUDE_DIR
            NORMALIZE OUTPUT_VARIABLE inc_src)
    else()
        set(inc_src "${CMAKE_CURRENT_SOURCE_DIR}/include")
    endif()
    set(inc_gen "${CMAKE_CURRENT_BINARY_DIR}/include")

    if(ARG_ASM_SOURCES AND NOT ARG_SOURCES)
        message(FATAL_ERROR
            "ctrlwork_add_module(${name}): ASM_SOURCES needs at least one C++ file in SOURCES")
    endif()

    if(ARG_SOURCES)
        add_library(${tgt} ${ARG_SOURCES})
        set(compiled TRUE)
        set(scope PUBLIC)
    else()
        add_library(${tgt} INTERFACE)
        set(compiled FALSE)
        set(scope INTERFACE)
        if(ARG_PRIVATE_DEPS)
            message(FATAL_ERROR
                "ctrlwork_add_module(${name}): PRIVATE_DEPS needs SOURCES")
        endif()
    endif()

    if(ARG_ASM_SOURCES)
        ctrlwork_target_add_fasm(${tgt}
            SOURCES ${ARG_ASM_SOURCES}
            DEPENDS ${ARG_ASM_DEPENDS})
    endif()

    if(ARG_ASM_EXPORTS)
        ctrlwork_target_export_asm_symbols(${tgt} ${ARG_ASM_EXPORTS})
    endif()

    add_library(${CTRLWORK_PACKAGE_NAME}::${ARG_ALIAS} ALIAS ${tgt})
    set_target_properties(${tgt} PROPERTIES EXPORT_NAME ${ARG_ALIAS})

    ctrlwork_configure_target(${tgt})

    target_include_directories(${tgt} ${scope}
        "$<BUILD_INTERFACE:${inc_src}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

    if(compiled)
        generate_export_header(${tgt}
            BASE_NAME ${base}
            EXPORT_FILE_NAME "${inc_gen}/ctrlwork/${name}/export.h")

        target_include_directories(${tgt} PUBLIC "$<BUILD_INTERFACE:${inc_gen}>")

        # Static builds must not use dllimport/visibility attributes.
        get_target_property(_type ${tgt} TYPE)
        if(_type STREQUAL "STATIC_LIBRARY")
            target_compile_definitions(${tgt} PUBLIC ${base}_STATIC_DEFINE)
        endif()

        set_target_properties(${tgt} PROPERTIES
            VERSION   ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR})
    endif()

    if(ARG_PUBLIC_DEPS)
        target_link_libraries(${tgt} ${scope} ${ARG_PUBLIC_DEPS})
    endif()
    if(ARG_PRIVATE_DEPS)
        target_link_libraries(${tgt} PRIVATE ${ARG_PRIVATE_DEPS})
    endif()

    set_property(GLOBAL APPEND PROPERTY CTRLWORK_MODULE_TARGETS ${tgt})

    if(CTRLWORK_INSTALL)
        install(TARGETS ${tgt}
            EXPORT  ${CTRLWORK_PACKAGE_NAME}Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        if(EXISTS "${inc_src}")
            install(DIRECTORY "${inc_src}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
        if(compiled)
            install(DIRECTORY "${inc_gen}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
    endif()
endfunction()
