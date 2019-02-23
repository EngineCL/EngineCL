message("src: ${CMAKE_CURRENT_SOURCE_DIR}")
message("root_dir: ${ROOT_DIR}")
message("src_dir: ${SRC_DIR}")
message("deps_dir: ${DEPS_DIR}")

set(Entry "examples")
set(Examples_Library "${Entry}" PARENT_SCOPE)

set (Needed_Libraries
  "<CL/cl.h>"
  "<pthread.h>"
  "<omp.h>"
  "<math.h>"
)

include_directories ("${SRC_DIR}")

include(SetLibraries)

set(SOURCES
  common/io.cpp
  tier-2/main.cpp
  tier-2/saxpy.cpp
)

set(HEADERS
  common/io.hpp
  tier-2/saxpy.hpp
)

IncludeLibraries("${Entry}" "${Needed_Libraries}")

add_library("${Entry}" SHARED ${SOURCES} ${HEADERS})

LinkLibraries("${Entry}" "${Needed_Libraries}")

target_link_libraries("${Entry}" "${Base_Library}")
