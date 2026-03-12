# 项目设置、子模块、add_subdirectory(src)

cmake_minimum_required(VERSION 4.0)
project(DearEngine)

# 安装后的 exe 从“自己所在目录”找 dylib，方便打包发给别人
set(CMAKE_INSTALL_RPATH "@executable_path")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

include(cmake)

# 子模块与 ImGui 路径
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui-docking)

add_subdirectory(include/SDL EXCLUDE_FROM_ALL)
# 把 SDL3 动态库装到 bin（目标名是 SDL3-shared，不是 SDL3）
if(TARGET SDL3-shared)
  install(TARGETS SDL3-shared
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin)
endif()

add_subdirectory(include/yaml-cpp)

set(GLM_ENABLE_CXX_20 ON)
add_subdirectory(include/glm EXCLUDE_FROM_ALL)

# 应用在 src 里单独管理
add_subdirectory(src)
