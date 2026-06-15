function(bbb_dmx_git_output output_variable working_directory)
    execute_process(
        COMMAND git ${ARGN}
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE command_output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE command_result
    )
    if(command_result EQUAL 0 AND NOT command_output STREQUAL "")
        set(${output_variable} "${command_output}" PARENT_SCOPE)
    else()
        set(${output_variable} "unknown" PARENT_SCOPE)
    endif()
endfunction()

function(bbb_dmx_git_dirty output_variable working_directory)
    execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE command_output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE command_result
    )
    if(NOT command_result EQUAL 0)
        set(${output_variable} "unknown" PARENT_SCOPE)
    elseif(command_output STREQUAL "")
        set(${output_variable} "clean" PARENT_SCOPE)
    else()
        set(${output_variable} "dirty" PARENT_SCOPE)
    endif()
endfunction()

function(bbb_dmx_escape_cpp_string output_variable input_value)
    string(REPLACE "\\" "\\\\" escaped_value "${input_value}")
    string(REPLACE "\"" "\\\"" escaped_value "${escaped_value}")
    set(${output_variable} "${escaped_value}" PARENT_SCOPE)
endfunction()

bbb_dmx_git_output(BBB_DMX_GIT_COMMIT "${SOURCE_DIR}" rev-parse --short=12 HEAD)
bbb_dmx_git_output(BBB_DMX_CORE_GIT_COMMIT "${CORE_DIR}" rev-parse --short=12 HEAD)
bbb_dmx_git_dirty(BBB_DMX_GIT_DIRTY "${SOURCE_DIR}")
bbb_dmx_git_dirty(BBB_DMX_CORE_GIT_DIRTY "${CORE_DIR}")
string(TIMESTAMP BBB_DMX_BUILD_TIME_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)

if(NOT DEFINED PACKAGE_VERSION OR PACKAGE_VERSION STREQUAL "")
    set(PACKAGE_VERSION "unknown")
endif()

set(BBB_DMX_COMBINED_DIRTY "${BBB_DMX_GIT_DIRTY}")
if(NOT BBB_DMX_CORE_GIT_DIRTY STREQUAL "clean")
    set(BBB_DMX_COMBINED_DIRTY "${BBB_DMX_GIT_DIRTY}+core-${BBB_DMX_CORE_GIT_DIRTY}")
endif()

bbb_dmx_escape_cpp_string(PACKAGE_VERSION_CPP "${PACKAGE_VERSION}")
bbb_dmx_escape_cpp_string(BBB_DMX_GIT_COMMIT_CPP "${BBB_DMX_GIT_COMMIT}")
bbb_dmx_escape_cpp_string(BBB_DMX_CORE_GIT_COMMIT_CPP "${BBB_DMX_CORE_GIT_COMMIT}")
bbb_dmx_escape_cpp_string(BBB_DMX_COMBINED_DIRTY_CPP "${BBB_DMX_COMBINED_DIRTY}")
bbb_dmx_escape_cpp_string(BBB_DMX_BUILD_TIME_UTC_CPP "${BBB_DMX_BUILD_TIME_UTC}")

set(header_content "#pragma once\n\nnamespace bbb::dmx::build_info {\n\ninline constexpr const char *package_version = \"${PACKAGE_VERSION_CPP}\";\ninline constexpr const char *git_commit = \"${BBB_DMX_GIT_COMMIT_CPP}\";\ninline constexpr const char *core_git_commit = \"${BBB_DMX_CORE_GIT_COMMIT_CPP}\";\ninline constexpr const char *git_dirty = \"${BBB_DMX_COMBINED_DIRTY_CPP}\";\ninline constexpr const char *build_time_utc = \"${BBB_DMX_BUILD_TIME_UTC_CPP}\";\n\n} // namespace bbb::dmx::build_info\n")

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
file(WRITE "${OUTPUT_FILE}" "${header_content}")
