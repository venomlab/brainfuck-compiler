include_guard(GLOBAL)

find_package(LLVM 18.1 REQUIRED CONFIG)
find_package(
    LLD 18.1 REQUIRED CONFIG
    HINTS "${LLVM_LIBRARY_DIRS}/cmake/lld"
)
find_package(
    CLI11 REQUIRED CONFIG
    HINTS "$ENV{VCPKG_ROOT}/installed/x64-linux"
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BFC_LIBXML2 REQUIRED libxml-2.0)
find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config-18
    HINTS "${LLVM_TOOLS_BINARY_DIR}"
    REQUIRED
)

set(
    BFC_LLVM_COMPONENTS
        all-targets
        lto
        option
        core
        support
        libdriver
        windowsdriver
        windowsmanifest
)
execute_process(
    COMMAND
        "${LLVM_CONFIG_EXECUTABLE}"
        --link-static
        --libs
        ${BFC_LLVM_COMPONENTS}
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
        ${BFC_LLVM_COMPONENTS}
    OUTPUT_VARIABLE BFC_LLVM_STATIC_SYSTEM_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
separate_arguments(
    BFC_LLVM_STATIC_SYSTEM_LIBRARIES
    NATIVE_COMMAND
    "${BFC_LLVM_STATIC_SYSTEM_LIBRARIES}"
)

function(bfc_configure_platform_options target)
    target_link_options(
        "${target}"
        INTERFACE
            "$<$<CONFIG:Release>:-static>"
    )
endfunction()

function(bfc_configure_platform_llvm target)
    target_include_directories(
        "${target}"
        SYSTEM PUBLIC
            "${LLVM_INCLUDE_DIRS}"
    )
    target_link_directories(
        "${target}"
        PUBLIC
            "${LLVM_LIBRARY_DIRS}"
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
            "$<$<NOT:$<CONFIG:Release>>:LLVM>"
            "$<$<NOT:$<CONFIG:Release>>:lldELF>"
            "$<$<NOT:$<CONFIG:Release>>:lldCOFF>"
            "$<$<CONFIG:Release>:$<TARGET_FILE:lldELF>>"
            "$<$<CONFIG:Release>:$<TARGET_FILE:lldCOFF>>"
            "$<$<CONFIG:Release>:$<TARGET_FILE:lldCommon>>"
    )
    foreach(BFC_LLVM_LIBRARY IN LISTS BFC_LLVM_STATIC_LIBRARIES)
        target_link_libraries(
            "${target}"
            PUBLIC
                "$<$<CONFIG:Release>:${BFC_LLVM_LIBRARY}>"
        )
    endforeach()
    foreach(BFC_LLVM_SYSTEM_LIBRARY IN LISTS BFC_LLVM_STATIC_SYSTEM_LIBRARIES)
        target_link_libraries(
            "${target}"
            PUBLIC
                "$<$<CONFIG:Release>:${BFC_LLVM_SYSTEM_LIBRARY}>"
        )
    endforeach()
    foreach(BFC_LIBXML2_STATIC_LIBRARY IN LISTS BFC_LIBXML2_STATIC_LIBRARIES)
        target_link_libraries(
            "${target}"
            PUBLIC
                "$<$<CONFIG:Release>:${BFC_LIBXML2_STATIC_LIBRARY}>"
        )
    endforeach()
endfunction()
