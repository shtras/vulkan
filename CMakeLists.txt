cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0092 NEW)

message("${CMAKE_CXX_FLAGS}")

project(Tutorials VERSION 1.0.0)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

function(ENABLE_EXTRA_CHECKS)
    message("Enabling extra warnings")
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy-10 -extra-arg=-Wno-unknown-warning-option -extra-arg=-Wno-format-nonliteral -extra-arg=-Wno-cast-align PARENT_SCOPE)
    if (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /permissive- /W4 /w14640 /w14242 /w14254 \
        /w14263 /w14265 /w14287 /we4289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 \
        /w14619 /w14826 /w14905 /w14906 /w14928 /nologo /sdl" PARENT_SCOPE)

    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -pedantic-errors -std=c++17 -Wall -Wextra -Wshadow \
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic \
            -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches \
            -Wlogical-op -Wuseless-cast" PARENT_SCOPE)
    endif ()
endfunction()

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Debug Build")
elseif (${CMAKE_BUILD_TYPE} MATCHES Release)
    message("Release Build")
endif ()
option(ENABLE_TESTS "Enable tests" ON)
if (ENABLE_TESTS)
    message("Enabling testing")
    enable_testing()
else ()
    message("Testing disabled")
endif ()
option(FAST_BUILD "Fast build" OFF)
if (NOT FAST_BUILD)
    enable_extra_checks()
else ()
    message("Not enabling extra warnings")
endif ()
option(BUILD_TOOLS "Enable tools" OFF)
message("Tools ${BUILD_TOOLS}")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb3")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror -O3 -ggdb3")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DRELEASE")

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

function(compile_shader target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "FORMAT" "SOURCES")
    foreach(source ${arg_SOURCES})
        add_custom_command(
            OUTPUT ${CMAKE_SOURCE_DIR}/run/shaders/${source}.spv
            DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${source}
            COMMAND
                ${glslc_executable}
                -MD -MF ${CMAKE_SOURCE_DIR}/run/shaders/${source}.d
                -o ${CMAKE_SOURCE_DIR}/run/shaders/${source}.spv
                ${CMAKE_SOURCE_DIR}/shaders/${source}
        )
        target_sources(${target} PRIVATE ${CMAKE_SOURCE_DIR}/run/shaders/${source}.spv)
    endforeach()
endfunction()

if (BUILD_TOOLS)
    message("Building additonal tools")
    add_subdirectory(tools)
endif ()

add_subdirectory(src)

if (ENABLE_TESTS)
    # add_subdirectory(tests)
endif ()
