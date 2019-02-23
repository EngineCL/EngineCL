# Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
# This file is part of EngineCL which is released under MIT License.
# See file LICENSE for full license details.

cmake_minimum_required(VERSION 3.3)

set(PROJECT EngineCL)
set(EXECUTABLE EngineCL)
project(${PROJECT})

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${ROOT_DIR}/src")
set(INCLUDE_DIR "${ROOT_DIR}/include")
set(DEPS_DIR "${ROOT_DIR}/deps")
set(EXAMPLES_DIR "${ROOT_DIR}/examples")

message("root_dir: ${ROOT_DIR}")
message("src_dir: ${ROOT_DIR}")

list(APPEND CMAKE_MODULE_PATH "${ROOT_DIR}/cmake/modules")

set (Needed_Libraries
  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
  "<math.h>"
)

include(OpenCL)
include(SetLibraries)
include(SetFlags)
# base library

set(CMAKE_INCLUDE_CURRENT_DIR ON)

configOpenCL()
message("OPENCL_VERSION: ${OPENCL_VERSION}")

SetFlags()

include_directories ("${SRC_DIR}")
include_directories ("${INCLUDE_DIR}")
include_directories ("${DEPS_DIR}")
include_directories ("${EXAMPLES_DIR}")

add_subdirectory(src)
message("Base_Library: ${Base_Library}")

add_subdirectory(examples)
message("Examples_Library: ${Examples_Library}")

set(Entry "EngineCL-tier2")
add_executable("${Entry}" examples/tier-2/main.cpp)
target_link_libraries("${Entry}"
  "${Base_Library}"
  "${Examples_Library}"
  "${Deps_Library}"
)
LinkLibraries("${Entry}" "${Needed_Libraries}")


option(TESTS "TESTS" OFF)
if (TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
