include_guard(GLOBAL)

find_package(LLVM 18.1 REQUIRED CONFIG)
find_package(LLD 18.1 REQUIRED CONFIG)
find_package(CLI11 REQUIRED CONFIG)

function(bfc_configure_platform_options target)
    target_link_options(
        "${target}"
        INTERFACE
            "$<$<CONFIG:Release>:-static>"
            "$<$<CONFIG:Release>:-static-libgcc>"
            "$<$<CONFIG:Release>:-static-libstdc++>"
    )
endfunction()

function(bfc_configure_platform_llvm target)
    target_include_directories(
        "${target}"
        SYSTEM PUBLIC
            "${LLVM_INCLUDE_DIRS}"
    )
    separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
    target_compile_options(
        "${target}"
        PUBLIC
            ${LLVM_DEFINITIONS_LIST}
    )
    target_link_libraries(
        "${target}"
        PUBLIC
            lldELF
            lldCOFF
    )
endfunction()
