cmake_minimum_required(VERSION 3.12)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0048 NEW)

project(Test1 VERSION 1.0.0)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

function(ENABLE_EXTRA_CHECKS)
    message("Enabling extra warnings")
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy-10 -extra-arg=-Wno-unknown-warning-option -extra-arg=-Wno-format-nonliteral -extra-arg=-Wno-cast-align PARENT_SCOPE)
    if (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /permissive- /W4 /w14640 /w14242 /w14254 /w14263 /w14265 /w14287 \
            /we4289 /w14296 /w14311 /w14545 /w14546 /w14547 /w14549 /w14555 /w14619 /w14826 /w14905 /w14906 /w14928")

    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -pedantic-errors -std=c++17 -Wall -Wextra -Wshadow \
            -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic \
            -Wconversion -Wsign-conversion -Wnull-dereference -Wdouble-promotion -Wformat=2" PARENT_SCOPE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches \
            -Wlogical-op -Wuseless-cast" PARENT_SCOPE)
    endif ()
endfunction()
set(CMAKE_VERBOSE_MAKEFILE TRUE)
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Debug Build")
elseif (${CMAKE_BUILD_TYPE} MATCHES Release)
    message("Release Build")
    set(CMAKE_VERBOSE_MAKEFILE TRUE)
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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
include_directories(${CMAKE_SOURCE_DIR}/src)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1")

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb3")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror -O3 -ggdb3")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DRELEASE")

# find_package(spdlog REQUIRED)

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

if (BUILD_TOOLS)
    message("Building additonal tools")
    add_subdirectory(tools)
endif ()

add_subdirectory(src)

if (ENABLE_TESTS)
    # add_subdirectory(tests)
endif ()
