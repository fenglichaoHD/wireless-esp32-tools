cmake_minimum_required(VERSION 3.5)
# Colored  Compiler Output with ninja
add_compile_options (-fdiagnostics-color=always)

# sync with git version to version.txt
function(write_git_version_to_file output_directory output_file)
    # use git versioning
    execute_process(
            COMMAND git describe --tags
            WORKING_DIRECTORY ${output_directory}
            OUTPUT_VARIABLE git_describe_output
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
    # Process the output to keep only version info
    string(REGEX REPLACE "-[0-9]*-g[0-9a-f]*" "" version_cleaned "${git_describe_output}")
    file(WRITE "${CMAKE_SOURCE_DIR}/${output_file}" "${version_cleaned}\n")
    message(STATUS "Version written to version.txt: ${version_cleaned}")
endfunction()

write_git_version_to_file(${CMAKE_SOURCE_DIR} version.txt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_COMPONENT_DIRS project_components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wireless_tools_esp32)
