cmake_minimum_required(VERSION 3.10)

project(SynthCore LANGUAGES CXX)

# Find all library source files
file(GLOB_RECURSE LIB_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/lib/*/src/*.cpp
)

# Find all library include directories
file(GLOB LIB_INCLUDE_DIRS
	LIST_DIRECTORIES true
	${CMAKE_CURRENT_SOURCE_DIR}/lib/*/src
)

add_library(SynthCore
	src/SynthCore.cpp
	${LIB_SOURCES}
)

target_include_directories(SynthCore PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${LIB_INCLUDE_DIRS}
)

add_executable(example
	examples/example.cpp
)

target_link_libraries(example PRIVATE SynthCore)