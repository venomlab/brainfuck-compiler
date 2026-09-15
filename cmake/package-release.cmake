cmake_minimum_required(VERSION 3.28)

foreach(required_variable IN ITEMS BFC_LINUX_BINARY BFC_WINDOWS_BINARY BFC_CMAKE_CACHE BFC_DIST_DIR)
    if (NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif()
endforeach()

get_filename_component(dist_name "${BFC_DIST_DIR}" NAME)
if (NOT dist_name STREQUAL "dist")
    message(FATAL_ERROR "BFC_DIST_DIR must point to a directory named dist")
endif()

foreach(binary IN ITEMS "${BFC_LINUX_BINARY}" "${BFC_WINDOWS_BINARY}")
    if (NOT EXISTS "${binary}")
        message(FATAL_ERROR "Release binary does not exist: ${binary}")
    endif()
endforeach()
if (NOT EXISTS "${BFC_CMAKE_CACHE}")
    message(FATAL_ERROR "CMake cache does not exist: ${BFC_CMAKE_CACHE}")
endif()

file(
    STRINGS "${BFC_CMAKE_CACHE}"
    version_entries
    REGEX "^CMAKE_PROJECT_VERSION:STATIC="
)
list(LENGTH version_entries version_entry_count)
if (NOT version_entry_count EQUAL 1)
    message(FATAL_ERROR "Could not read a unique project version from ${BFC_CMAKE_CACHE}")
endif()
list(GET version_entries 0 version_entry)
string(REGEX REPLACE "^CMAKE_PROJECT_VERSION:STATIC=" "" release_version "${version_entry}")
if (release_version STREQUAL "")
    message(FATAL_ERROR "Project version is empty")
endif()

set(staging_dir "${BFC_DIST_DIR}.tmp")
set(backup_dir "${BFC_DIST_DIR}.backup")
set(linux_staging_dir "${staging_dir}/linux")
set(windows_staging_dir "${staging_dir}/windows")
set(linux_archive_name "bfc-${release_version}-x86_64-linux.tar.gz")
set(windows_archive_name "bfc-${release_version}-x86_64-windows.zip")
set(linux_archive "${staging_dir}/${linux_archive_name}")
set(windows_archive "${staging_dir}/${windows_archive_name}")

file(REMOVE_RECURSE "${staging_dir}")
if (EXISTS "${backup_dir}")
    if (EXISTS "${BFC_DIST_DIR}")
        file(REMOVE_RECURSE "${backup_dir}")
    else()
        file(RENAME "${backup_dir}" "${BFC_DIST_DIR}" RESULT recovery_result)
        if (NOT recovery_result STREQUAL "0")
            message(FATAL_ERROR "Could not recover previous dist directory: ${recovery_result}")
        endif()
    endif()
endif()
file(MAKE_DIRECTORY "${linux_staging_dir}" "${windows_staging_dir}")

file(COPY_FILE "${BFC_LINUX_BINARY}" "${staging_dir}/bfc")
file(COPY_FILE "${BFC_WINDOWS_BINARY}" "${staging_dir}/bfc.exe")
file(COPY_FILE "${BFC_LINUX_BINARY}" "${linux_staging_dir}/bfc")
file(COPY_FILE "${BFC_WINDOWS_BINARY}" "${windows_staging_dir}/bfc.exe")
file(
    CHMOD "${staging_dir}/bfc" "${linux_staging_dir}/bfc"
    PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E tar czf "${linux_archive}" --format=gnutar -- bfc
    WORKING_DIRECTORY "${linux_staging_dir}"
    RESULT_VARIABLE linux_archive_result
    ERROR_VARIABLE linux_archive_error
)
if (NOT linux_archive_result EQUAL 0)
    message(FATAL_ERROR "Could not create Linux archive: ${linux_archive_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E tar cf "${windows_archive}" --format=zip -- bfc.exe
    WORKING_DIRECTORY "${windows_staging_dir}"
    RESULT_VARIABLE windows_archive_result
    ERROR_VARIABLE windows_archive_error
)
if (NOT windows_archive_result EQUAL 0)
    message(FATAL_ERROR "Could not create Windows archive: ${windows_archive_error}")
endif()

file(REMOVE_RECURSE "${linux_staging_dir}" "${windows_staging_dir}")
file(SHA256 "${linux_archive}" linux_archive_sha256)
file(SHA256 "${windows_archive}" windows_archive_sha256)
file(
    WRITE "${staging_dir}/SHA256SUMS"
    "${linux_archive_sha256}  ${linux_archive_name}\n"
    "${windows_archive_sha256}  ${windows_archive_name}\n"
)

set(had_previous_dist FALSE)
if (EXISTS "${BFC_DIST_DIR}")
    file(RENAME "${BFC_DIST_DIR}" "${backup_dir}" RESULT backup_result)
    if (NOT backup_result STREQUAL "0")
        message(FATAL_ERROR "Could not back up existing dist directory: ${backup_result}")
    endif()
    set(had_previous_dist TRUE)
endif()

file(RENAME "${staging_dir}" "${BFC_DIST_DIR}" RESULT install_result)
if (NOT install_result STREQUAL "0")
    if (had_previous_dist)
        file(RENAME "${backup_dir}" "${BFC_DIST_DIR}" RESULT restore_result)
        if (NOT restore_result STREQUAL "0")
            message(FATAL_ERROR "Could not install release or restore previous dist: ${restore_result}")
        endif()
    endif()
    message(FATAL_ERROR "Could not install release artifacts: ${install_result}")
endif()

if (had_previous_dist)
    file(REMOVE_RECURSE "${backup_dir}")
endif()

message(STATUS "Release artifacts written to ${BFC_DIST_DIR}")
