cmake_minimum_required(VERSION 3.19)
project(bbb.dmx VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bbb_dmx_common INTERFACE)
target_include_directories(bbb_dmx_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

enable_testing()
add_executable(bbb_dmx_common_tests
    tests/bbb_dmx_common_tests.cpp
)
target_link_libraries(bbb_dmx_common_tests PRIVATE bbb_dmx_common)
add_test(NAME bbb_dmx_common_tests COMMAND bbb_dmx_common_tests)

set(BBB_DMX_EXTERNALS_DEFAULT OFF)
if(DEFINED C74_MIN_API_DIR OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/min-api")
    set(BBB_DMX_EXTERNALS_DEFAULT ON)
endif()
option(BBB_DMX_BUILD_EXTERNALS "Build Max external objects" ${BBB_DMX_EXTERNALS_DEFAULT})

if(BBB_DMX_BUILD_EXTERNALS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bbb_external.cmake)

    set(C74_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/externals)

    if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
    endif()

    macro(SUBDIRLIST result current_directory)
        file(GLOB children RELATIVE ${current_directory} ${current_directory}/*)
        set(directory_list "")
        foreach(child ${children})
            if(IS_DIRECTORY ${current_directory}/${child})
                list(APPEND directory_list ${child})
            endif()
        endforeach()
        set(${result} ${directory_list})
    endmacro()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects")
        SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/source/projects)
        foreach(project_directory ${PROJECT_DIRS})
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_directory}/CMakeLists.txt")
                message("Generating: ${project_directory}")
                add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/projects/${project_directory})
            endif()
        endforeach()
    endif()
endif()

set(CMAKE_SUPPRESS_REGENERATION true)
