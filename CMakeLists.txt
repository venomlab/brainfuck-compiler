cmake_minimum_required(VERSION 3.28)


project(
    brainfuck-compiler
    VERSION 0.0.0
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

find_package(LLVM 18.1 REQUIRED CONFIG)
find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config-18
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
)

execute_process(
    COMMAND
        "${LLVM_CONFIG_EXECUTABLE}"
        --link-static
        --libs
        nativecodegen
        x86asmparser
        core
        support
    OUTPUT_VARIABLE BFC_LLVM_STATIC_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
separate_arguments(
    BFC_LLVM_STATIC_LIBRARIES
    NATIVE_COMMAND
    "${BFC_LLVM_STATIC_LIBRARIES}"
)

execute_process(
    COMMAND
        "${LLVM_CONFIG_EXECUTABLE}"
        --link-static
        --system-libs
        nativecodegen
        x86asmparser
        core
        support
    OUTPUT_VARIABLE BFC_LLVM_STATIC_SYSTEM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
separate_arguments(
    BFC_LLVM_STATIC_SYSTEM_LIBRARIES
    NATIVE_COMMAND
    "${BFC_LLVM_STATIC_SYSTEM_LIBRARIES}"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

add_library(bfc_options INTERFACE)


target_compile_features(
    bfc_options
    INTERFACE
        cxx_std_20
)
target_link_options(
    bfc_options
    INTERFACE
        "$<$<CONFIG:Release>:-static>"
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
target_include_directories(
    bfc_llvm
    SYSTEM PUBLIC
        "${LLVM_INCLUDE_DIRS}"
)
target_link_directories(
    bfc_llvm
    PUBLIC
        "${LLVM_LIBRARY_DIRS}"
)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(
    bfc_llvm
    PUBLIC
        ${LLVM_DEFINITIONS_LIST}
)
target_link_libraries(
    bfc_llvm
    PUBLIC
        bfc_core
        "$<$<NOT:$<CONFIG:Release>>:LLVM>"
    PRIVATE
        bfc_warnings
)
foreach(BFC_LLVM_LIBRARY IN LISTS BFC_LLVM_STATIC_LIBRARIES)
    target_link_libraries(
        bfc_llvm
        PUBLIC
            "$<$<CONFIG:Release>:${BFC_LLVM_LIBRARY}>"
    )
endforeach()
foreach(BFC_LLVM_SYSTEM_LIBRARY IN LISTS BFC_LLVM_STATIC_SYSTEM_LIBRARIES)
    target_link_libraries(
        bfc_llvm
        PUBLIC
            "$<$<CONFIG:Release>:${BFC_LLVM_SYSTEM_LIBRARY}>"
    )
endforeach()

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
enable_clang_tidy(bfc_core)
enable_clang_tidy(bfc_llvm)
