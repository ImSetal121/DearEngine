cmake_minimum_required(VERSION 4.0)
project(LearnCPP)

set(CMAKE_CXX_STANDARD 26)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in submodule/SDL
add_subdirectory(submodule/SDL EXCLUDE_FROM_ALL)

add_executable(LearnCPP main.cpp)

# Link to the actual SDL3 library.
target_link_libraries(LearnCPP PRIVATE SDL3::SDL3)
