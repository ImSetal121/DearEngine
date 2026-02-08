# 项目设置、子模块、add_subdirectory(src)

cmake_minimum_required(VERSION 4.0)
project(LearnCPP)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# 子模块与 ImGui 路径
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodule/imgui)
add_subdirectory(submodule/SDL EXCLUDE_FROM_ALL)

# 应用在 src 里单独管理
add_subdirectory(src)
