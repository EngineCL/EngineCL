# Copyright (c) 2018    ATC (University of Cantabria) <nozalr@unican.es>
# This file is part of EngineCL which is released under MIT License.
# See file LICENSE for full license details.

message("src: ${CMAKE_CURRENT_SOURCE_DIR}")
message("root_dir: ${ROOT_DIR}")
message("src_dir: ${SRC_DIR}")
message("deps_dir: ${DEPS_DIR}")
message("include_dir: ${INCLUDE_DIR}")

set(Entry "EngineCL-core")
set(Base_Library "${Entry}" PARENT_SCOPE)

set (Needed_Libraries
  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
)

include_directories ("${SRC_DIR}")
include_directories ("${DEPS_DIR}")
include(SetLibraries)

set(SOURCES
        Buffer.cpp
        Runtime.cpp
        schedulers/Static.cpp
        schedulers/Dynamic.cpp
        Device.cpp
        CLUtils.cpp
        Inspector.cpp
)

set(HEADERS
  ${INCLUDE_DIR}/config.hpp
  ${INCLUDE_DIR}/Buffer.hpp
  ${INCLUDE_DIR}/EngineCL.hpp
  ${INCLUDE_DIR}/Semaphore.hpp
  ${INCLUDE_DIR}/NDRange.hpp
  ${INCLUDE_DIR}/Work.hpp
  ${INCLUDE_DIR}/Runtime.hpp
  ${INCLUDE_DIR}/schedulers/Static.hpp
  ${INCLUDE_DIR}/schedulers/Dynamic.hpp
  ${INCLUDE_DIR}/Scheduler.hpp
  ${INCLUDE_DIR}/Device.hpp
  ${INCLUDE_DIR}/CLUtils.hpp
  ${INCLUDE_DIR}/Inspector.hpp
)

IncludeLibraries("${Entry}" "${Needed_Libraries}")

add_library("${Entry}" SHARED ${HEADERS} ${SOURCES})

LinkLibraries("${Entry}" "${Needed_Libraries}")
