include_guard(GLOBAL)

option(CTRLWORK_WARNINGS_AS_ERRORS "Treat warnings as errors in ctrlwork targets." OFF)

# ctrlwork_configure_target(<target>)
#
# Applies the project-wide defaults to one of OUR targets. It is opt-in per
# target on purpose: nothing leaks into third-party code or into a parent
# project when ctrlwork is consumed via add_subdirectory/FetchContent, and no
# helper target ends up in the install export set.
function(ctrlwork_configure_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "ctrlwork_configure_target: '${target}' is not a target")
    endif()

    get_target_property(_type "${target}" TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        target_compile_features("${target}" INTERFACE cxx_std_23)
        return()
    endif()

    # Language level: propagates to consumers of libraries.
    target_compile_features("${target}" PUBLIC cxx_std_23)

    set_target_properties("${target}" PROPERTIES
        CXX_EXTENSIONS            OFF
        CXX_VISIBILITY_PRESET     hidden
        C_VISIBILITY_PRESET       hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    # Warnings / encoding. MSVC is true for both cl.exe and clang-cl.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(_opts /W4 /permissive- /utf-8 /Zc:__cplusplus /Zc:preprocessor)
        set(_werror /WX)
    elseif(MSVC) # clang-cl: MSVC-style command line, Clang front end
        set(_opts /W4 /utf-8)
        set(_werror /WX)
    else()       # GCC, Clang, AppleClang
        set(_opts -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
        set(_werror -Werror)
    endif()

    if(CTRLWORK_WARNINGS_AS_ERRORS)
        list(APPEND _opts ${_werror})
    endif()

    target_compile_options("${target}" PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX,C>:${_opts}>")

    if(WIN32)
        target_compile_definitions("${target}" PRIVATE
            NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()
endfunction()
