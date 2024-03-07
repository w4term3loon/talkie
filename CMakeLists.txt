cmake_minimum_required(VERSION 3.16)
project(talkie)

if(NOT TARGET ncwrap)
	add_subdirectory(
		"${CMAKE_CURRENT_SOURCE_DIR}/../ncwrap" 
		"${CMAKE_CURRENT_BINARY_DIR}/ncwrap")
endif()

add_executable(verbose
	"src/verbose.c"
)
target_link_libraries(verbose ncwrap)

add_executable(listener
	"src/listener.c"
)
target_link_libraries(listener ncwrap)


