cmake_minimum_required(VERSION 3.30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CtrlWorkExamples LANGUAGES CXX)
    enable_testing()
    find_package(CtrlWork REQUIRED)
endif()

function(ctrlwork_add_example name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "NO_TEST" "" "SOURCES;LIBS")
    if(NOT ARG_SOURCES OR NOT ARG_LIBS)
        message(FATAL_ERROR "ctrlwork_add_example(${name}): SOURCES and LIBS are required")
    endif()

    set(tgt "ctrlwork_example_${name}")
    add_executable(${tgt} ${ARG_SOURCES})
    set_target_properties(${tgt} PROPERTIES OUTPUT_NAME "example_${name}")
    target_link_libraries(${tgt} PRIVATE ${ARG_LIBS})

    if(COMMAND ctrlwork_configure_target)
        ctrlwork_configure_target(${tgt})
    endif()

    if(NOT ARG_NO_TEST)
        add_test(NAME example.${name} COMMAND ${tgt})
        set_tests_properties(example.${name} PROPERTIES LABELS "example")
    endif()
endfunction()

add_subdirectory(bigint)
add_subdirectory(bcd_packed)
add_subdirectory(matrix)
