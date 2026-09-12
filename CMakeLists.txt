cmake_minimum_required(VERSION 3.28)


project(
    brainfuck-compiler
    VERSION 0.0.0
    LANGUAGES CXX
)
option(
    ENABLE_ASAN_UBSAN
    "Enable address sanitizer and undefinedbehavior sanitizer"
    OFF
)
option(
    ENABLE_TSAN
    "Enable thread sanitizer"
    OFF
)
option(
    ENABLE_TIDY
    "Enable clang-tidy"
    OFF
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_library(bfc_options INTERFACE)


target_compile_features(
    bfc_options
    INTERFACE
        cxx_std_23
)

add_library(bfc_warnings INTERFACE)

target_compile_options(
    bfc_warnings
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Werror
)

if (ENABLE_ASAN_UBSAN)
    target_compile_options(
        bfc_options
        INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=undefined
    )
    target_link_options(
        bfc_options
        INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=undefined
    )
endif()
if (ENABLE_TSAN)
    target_compile_options(
        bfc_options
        INTERFACE
            -fsanitize=thread
            -fno-omit-frame-pointer
    )
    target_link_options(
        bfc_options
        INTERFACE
            -fsanitize=thread
            -fno-omit-frame-pointer
    )
endif()

# Library
file(
    GLOB_RECURSE BFC_CORE_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/core/*.cpp"
)
add_library(
    bfc_core
    STATIC
        ${BFC_CORE_SOURCES}
)
target_compile_options(
    bfc_core
    PUBLIC
)
target_include_directories(
    bfc_core
    PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(
    bfc_core
    PUBLIC
        bfc_options
    PRIVATE
        bfc_warnings
)

# Executable
file(
    GLOB_RECURSE BFC_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/bfc/*.cpp"
)
add_executable(
    bfc
    ${BFC_SOURCES}
)
target_link_libraries(
    bfc
    PRIVATE
        bfc_core
        bfc_warnings
)
### static linking
target_link_options(
    bfc
    PRIVATE
)
# TESTS
include(CTest)
enable_testing()
find_package(GTest REQUIRED)
include(GoogleTest)
if (BUILD_TESTING)
    file(GLOB_RECURSE BFC_TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )
    add_executable(
        bfc_tests
        ${BFC_TEST_SOURCES}
    )
    target_link_libraries(
        bfc_tests
        PRIVATE
            bfc_core
            bfc_warnings
            GTest::gtest_main
    )
    gtest_discover_tests(bfc_tests)
endif()

# CLANG TIDY
if(ENABLE_TIDY)
    find_program(
        CLANG_TIDY_EXECUTABLE
        NAMES clang-tidy
        REQUIRED
    )
endif()
function(enable_clang_tidy target_name)
    if(ENABLE_TIDY)
        set_target_properties(
            "${target_name}"
            PROPERTIES
                CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}"
        )
    endif()
endfunction()

enable_clang_tidy(bfc)
enable_clang_tidy(bfc_core)
