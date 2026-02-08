cmake_minimum_required(VERSION 4.0)
project(ExampleSDL3GpuTriangle)

set(CMAKE_CXX_STANDARD 26)

find_program(GLSLC_PROG glslc)
find_program(SPIRV_CROSS_PROG spirv-cross)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# 让 SDL 作为 subdirectory 构建时能找到 LINK_DEPENDS 的 .sym 文件
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/dynapi")
configure_file(
  "${CMAKE_SOURCE_DIR}/SDL/src/dynapi/SDL_dynapi.sym"
  "${CMAKE_BINARY_DIR}/src/dynapi/SDL_dynapi.sym"
  COPYONLY
)

add_executable(ExampleSDL3GpuTriangle main.cpp shader.vert shader.frag vert.spv frag.spv)
add_subdirectory(SDL EXCLUDE_FROM_ALL)
target_link_libraries(ExampleSDL3GpuTriangle PRIVATE SDL3::SDL3)
set_target_properties(ExampleSDL3GpuTriangle
    PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include(cmake/compile_shader.cmake)

compile_shader_to_spv(shader.vert vert.spv)
compile_shader_to_spv(shader.frag frag.spv)
compile_shader_to_msl(vert.spv vert.msl)
compile_shader_to_msl(frag.spv frag.msl)


