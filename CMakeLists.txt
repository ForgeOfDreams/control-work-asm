cmake_minimum_required(VERSION 3.30)

#=============================
# Project / Package metadata
#=============================

set(CTRLWORK_PACKAGE_NAME "CtrlWork")
set(CTRLWORK_VERSION_MAJOR 0)
set(CTRLWORK_VERSION_MINOR 1)
set(CTRLWORK_VERSION_BUILD 0)
set(CTRLWORK_DESCRIPTION "")

project(CtrlWork
    VERSION ${CTRLWORK_VERSION_MAJOR}.${CTRLWORK_VERSION_MINOR}.${CTRLWORK_VERSION_BUILD}
    DESCRIPTION CTRLWORK_DESCRIPTION
    LANGUAGES CXX C
)

set(CTRLWORK_VERSION_STRING ${PROJECT_VERSION})

if(PROJECT_IS_TOP_LEVEL AND CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds are not allowed. Use: cmake --preset <name>"
    )
endif()


#=============================
# Language setup
#=============================

enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#=============================
# Configurable options
#=============================

include(CMakeDependentOption)
include(GNUInstallDirs)

string(COMPARE EQUAL ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} PROJECT_IS_TOP_LEVEL)

option(CTRLWORK_BUILD "Build ctrlwork targets." ON)
cmake_dependent_option(CTRLWORK_BUILD_TESTS
    "Build tests."    ${PROJECT_IS_TOP_LEVEL} "CTRLWORK_BUILD" OFF)
cmake_dependent_option(CTRLWORK_BUILD_EXAMPLES
    "Build examples." ${PROJECT_IS_TOP_LEVEL} "CTRLWORK_BUILD" OFF)
option(CTRLWORK_INSTALL "Generate install rules." ${PROJECT_IS_TOP_LEVEL})


if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH 
    ${PROJECT_SOURCE_DIR}/cmake/modules
)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)


#=============================
# Subdirectory
#=============================

if (CTRLWORK_BUILD)
    include(CtrlworkTargetDefaults)

    add_subdirectory(src)
endif()

#=============================
# Test
#=============================

if(CTRLWORK_BUILD AND CTRLWORK_BUILD_TESTS)    
    add_subdirectory(tests)
endif()

#=============================
# Examples
#=============================

if(CTRLWORK_BUILD AND CTRLWORK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
