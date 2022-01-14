# CMakeList.txt : CMake project for stack_vector_test, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(srcs
	"${CMAKE_CURRENT_SOURCE_DIR}/tests/stack_vector_test.cpp"
)
set(hdrs
	"${CMAKE_CURRENT_SOURCE_DIR}/include/stack_vector.h"
)

# Add source to this project's executable.
add_executable (stack_vector_test ${srcs} ${hdrs})
target_include_directories(stack_vector_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
set_property (TARGET stack_vector_test PROPERTY CXX_STANDARD 20)

# TODO: Add tests and install targets if needed.
