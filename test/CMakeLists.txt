cmake_minimum_required(VERSION 3.3)

set(TESTS
  Semaphore.cpp
  tests.cpp
)

set(TestExecutable "test-${EXECUTABLE}")

message("test: ${CMAKE_CURRENT_SOURCE_DIR}")
message("src: ${SRC_DIR}")
message("root_dir: ${ROOT_DIR}")

include_directories("${SRC_DIR}")
include_directories("${DEPS_DIR}/Catch")

add_executable(${TestExecutable} ${TESTS} ${HEADERS})

target_link_libraries(${TestExecutable} ${Base_Library})

LinkLibraries("${TestExecutable}" "${Needed_Libraries}")

add_test(NAME EngineCLTest COMMAND ${TestExecutable})
