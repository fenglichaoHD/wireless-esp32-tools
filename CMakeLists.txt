cmake_minimum_required(VERSION 3.5)
# Colored  Compiler Output with ninja
add_compile_options (-fdiagnostics-color=always)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(EXTRA_COMPONENT_DIRS project_components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wireless_tools_esp32)
