cmake_minimum_required(VERSION 3.28)


project(
    brainfuck-compiler
    VERSION 1.0.0
    LANGUAGES C CXX
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

if (NOT BFC_PLATFORM)
    message(FATAL_ERROR "BFC_PLATFORM must be set by the active toolchain")
endif()
set(BFC_PLATFORM_FILE "${PROJECT_SOURCE_DIR}/cmake/platforms/${BFC_PLATFORM}.cmake")
if (NOT EXISTS "${BFC_PLATFORM_FILE}")
    message(FATAL_ERROR "Unknown BFC platform: ${BFC_PLATFORM}")
endif()
include("${BFC_PLATFORM_FILE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_library(bfc_options INTERFACE)


target_compile_features(
    bfc_options
    INTERFACE
        cxx_std_20
)
bfc_configure_platform_options(bfc_options)

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
    "${PROJECT_SOURCE_DIR}/src/bfc/core/*.cpp"
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

# CLI
file(
    GLOB_RECURSE BFC_CLI_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/bfc/cli/*.cpp"
)
add_library(
    bfc_cli
    STATIC
        ${BFC_CLI_SOURCES}
)
target_include_directories(
    bfc_cli
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
target_compile_definitions(
    bfc_cli
    PRIVATE
        BFC_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(
    bfc_cli
    PUBLIC
        bfc_options
        CLI11::CLI11
    PRIVATE
        bfc_warnings
)

# LLVM backend
file(
    GLOB_RECURSE BFC_LLVM_SOURCES
    CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/bfc/llvm/*.cpp"
)
add_library(
    bfc_llvm
    STATIC
        ${BFC_LLVM_SOURCES}
)
target_include_directories(
    bfc_llvm
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(
    bfc_llvm
    PUBLIC
        bfc_core
    PRIVATE
        bfc_warnings
)
bfc_configure_platform_llvm(bfc_llvm)

# Executable
file(
    GLOB BFC_SOURCES
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
        bfc_cli
        bfc_llvm
        bfc_warnings
)
# TESTS
include(CTest)
enable_testing()
if (BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    file(GLOB BFC_TEST_SOURCES
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

    file(GLOB_RECURSE BFC_CLI_TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli/*.cpp"
    )
    add_executable(
        bfc_cli_tests
        ${BFC_CLI_TEST_SOURCES}
    )
    target_link_libraries(
        bfc_cli_tests
        PRIVATE
            bfc_cli
            bfc_warnings
            GTest::gtest_main
    )
    gtest_discover_tests(bfc_cli_tests)

    add_test(
        NAME bfc_cli_integration
        COMMAND
            "${CMAKE_COMMAND}"
            "-DBFC_EXECUTABLE=$<TARGET_FILE:bfc>"
            "-DBFC_VERSION=${PROJECT_VERSION}"
            "-DTEST_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/cli-integration"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli/integration.cmake"
    )

    file(GLOB_RECURSE BFC_LLVM_TEST_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/llvm/*.cpp"
    )
    add_executable(
        bfc_llvm_tests
        ${BFC_LLVM_TEST_SOURCES}
    )
    target_link_libraries(
        bfc_llvm_tests
        PRIVATE
            bfc_llvm
            bfc_warnings
            GTest::gtest_main
    )
    gtest_discover_tests(bfc_llvm_tests)
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
enable_clang_tidy(bfc_cli)
enable_clang_tidy(bfc_core)
enable_clang_tidy(bfc_llvm)
