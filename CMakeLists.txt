# 项目设置、子模块、add_subdirectory(src)

cmake_minimum_required(VERSION 4.0)
project(DearEngine)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

include(cmake)

# 子模块与 ImGui 路径
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui-docking)
add_subdirectory(include/SDL EXCLUDE_FROM_ALL)
set(BUILD_SANDBOX OFF CACHE BOOL "Build cereal sandbox" FORCE)
add_subdirectory(include/cereal)
set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(include/glm EXCLUDE_FROM_ALL)

# 应用在 src 里单独管理
add_subdirectory(src)
