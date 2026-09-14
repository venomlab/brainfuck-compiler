cmake_minimum_required(VERSION 3.28)

if (NOT DEFINED BFC_EXECUTABLE)
    message(FATAL_ERROR "BFC_EXECUTABLE is required")
endif()
if (NOT DEFINED BFC_VERSION)
    message(FATAL_ERROR "BFC_VERSION is required")
endif()
if (NOT DEFINED TEST_DIRECTORY)
    message(FATAL_ERROR "TEST_DIRECTORY is required")
endif()

function(require_success name result error_output)
    if (NOT result EQUAL 0)
        message(FATAL_ERROR "${name} failed with ${result}: ${error_output}")
    endif()
endfunction()

function(expect_contains name contents expected)
    string(FIND "${contents}" "${expected}" position)
    if (position EQUAL -1)
        message(FATAL_ERROR "${name} does not contain '${expected}'")
    endif()
endfunction()

function(expect_elf name path)
    file(READ "${path}" magic LIMIT 4 HEX)
    if (NOT magic STREQUAL "7f454c46")
        message(FATAL_ERROR "${name} is not an ELF artifact")
    endif()
endfunction()

function(run_bfc output_variable)
    execute_process(
        COMMAND "${BFC_EXECUTABLE}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
    )
    require_success("bfc ${ARGN}" "${result}" "${error_output}")
    set("${output_variable}" "${output}" PARENT_SCOPE)
endfunction()

file(REMOVE_RECURSE "${TEST_DIRECTORY}")
file(MAKE_DIRECTORY "${TEST_DIRECTORY}")

set(source "${TEST_DIRECTORY}/hello.bf")
file(
    WRITE "${source}"
    "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>."
)

run_bfc(version_output --version)
string(STRIP "${version_output}" version_output)
if (NOT version_output STREQUAL "bfc ${BFC_VERSION}")
    message(FATAL_ERROR "Unexpected version: '${version_output}'")
endif()

execute_process(
    COMMAND "${BFC_EXECUTABLE}" --ir
    INPUT_FILE "${source}"
    RESULT_VARIABLE stdin_result
    OUTPUT_VARIABLE stdin_ir
    ERROR_VARIABLE stdin_error
)
require_success("IR from stdin" "${stdin_result}" "${stdin_error}")
expect_contains("IR from stdin" "${stdin_ir}" "define i32 @main()")

run_bfc(ir_by_flag --ir "${source}")
expect_contains("IR selected by --ir" "${ir_by_flag}" "define i32 @main()")

run_bfc(assembly_by_flag --asm "${source}")
expect_contains("assembly selected by --asm" "${assembly_by_flag}" "main")

set(object_by_flag "${TEST_DIRECTORY}/flag.o")
run_bfc(unused --obj -o "${object_by_flag}" "${source}")
expect_elf("object selected by --obj" "${object_by_flag}")

set(executable_by_flag "${TEST_DIRECTORY}/flag.out")
run_bfc(unused --exe -o "${executable_by_flag}" "${source}")
execute_process(
    COMMAND "${executable_by_flag}"
    RESULT_VARIABLE execution_result
    OUTPUT_VARIABLE program_output
    ERROR_VARIABLE execution_error
)
require_success("generated executable" "${execution_result}" "${execution_error}")
if (NOT program_output STREQUAL "Hello World!\n")
    message(FATAL_ERROR "Unexpected program output: '${program_output}'")
endif()

set(inferred_ir "${TEST_DIRECTORY}/inferred.ll")
run_bfc(unused -o "${inferred_ir}" "${source}")
file(READ "${inferred_ir}" inferred_ir_contents)
expect_contains("IR inferred from .ll" "${inferred_ir_contents}" "define i32 @main()")

set(inferred_assembly "${TEST_DIRECTORY}/inferred.s")
run_bfc(unused -o "${inferred_assembly}" "${source}")
file(READ "${inferred_assembly}" inferred_assembly_contents)
expect_contains("assembly inferred from .s" "${inferred_assembly_contents}" "main")

set(inferred_object "${TEST_DIRECTORY}/inferred.o")
run_bfc(unused -o "${inferred_object}" "${source}")
expect_elf("object inferred from .o" "${inferred_object}")

set(inferred_executable "${TEST_DIRECTORY}/inferred.out")
run_bfc(unused -o "${inferred_executable}" "${source}")
execute_process(
    COMMAND "${inferred_executable}"
    RESULT_VARIABLE inferred_execution_result
    OUTPUT_VARIABLE inferred_program_output
    ERROR_VARIABLE inferred_execution_error
)
require_success("executable inferred from .out" "${inferred_execution_result}" "${inferred_execution_error}")
if (NOT inferred_program_output STREQUAL "Hello World!\n")
    message(FATAL_ERROR "Unexpected inferred executable output: '${inferred_program_output}'")
endif()

set(cross_assembly "${TEST_DIRECTORY}/aarch64.s")
run_bfc(unused --asm --target aarch64-unknown-linux-gnu -o "${cross_assembly}" "${source}")
file(READ "${cross_assembly}" cross_assembly_contents)
expect_contains("AArch64 assembly" "${cross_assembly_contents}" "main")

set(cross_object "${TEST_DIRECTORY}/aarch64.o")
run_bfc(unused --obj --target aarch64-unknown-linux-gnu -o "${cross_object}" "${source}")
expect_elf("AArch64 object" "${cross_object}")
file(READ "${cross_object}" cross_machine OFFSET 18 LIMIT 2 HEX)
if (NOT cross_machine STREQUAL "b700")
    message(FATAL_ERROR "AArch64 object has unexpected ELF machine: ${cross_machine}")
endif()

set(embedded_executable "${TEST_DIRECTORY}/embedded.out")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env "PATH=/nonexistent"
        "${BFC_EXECUTABLE}" --exe -o "${embedded_executable}" "${source}"
    RESULT_VARIABLE embedded_result
    OUTPUT_VARIABLE embedded_output
    ERROR_VARIABLE embedded_error
)
require_success("executable generation without clang" "${embedded_result}" "${embedded_error}")
execute_process(
    COMMAND "${embedded_executable}"
    RESULT_VARIABLE embedded_execution_result
    OUTPUT_VARIABLE embedded_program_output
    ERROR_VARIABLE embedded_execution_error
)
require_success("embedded executable" "${embedded_execution_result}" "${embedded_execution_error}")
if (NOT embedded_program_output STREQUAL "Hello World!\n")
    message(FATAL_ERROR "Unexpected embedded executable output: '${embedded_program_output}'")
endif()

set(stdout_executable "${TEST_DIRECTORY}/stdout.out")
execute_process(
    COMMAND "${BFC_EXECUTABLE}" "${source}"
    RESULT_VARIABLE stdout_result
    OUTPUT_FILE "${stdout_executable}"
    ERROR_VARIABLE stdout_error
)
require_success("executable written to stdout" "${stdout_result}" "${stdout_error}")
expect_elf("executable written to stdout" "${stdout_executable}")
