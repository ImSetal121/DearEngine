# DearEngine 可执行文件（源、链接、include）

find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(ALL_FILES ${SRC_FILES} ${HEADER_FILES})

add_executable(DearEngine
        ${ALL_FILES}
        # glad加载库
        ${CMAKE_SOURCE_DIR}/include/glad/glad.c
        # ImGui 核心
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        # SDL3 + SDL_GPU3 后端
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

## 编译跨平台shader(glsl->spv->msl)
#include(${CMAKE_SOURCE_DIR}/cmake/compile_all_shaders.cmake)
#add_dependencies(DearEngine compile_all_shaders)

# Link to the library.
target_link_libraries(DearEngine PRIVATE
        SDL3::SDL3
        glm::glm
        yaml-cpp::yaml-cpp
        ${OPENGL_LIBRARIES}
)

target_include_directories(DearEngine PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${OPENGL_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

# -----------------------------------------------------------------------------
# 编译期注入项目内容根目录 CONTENT_ROOT（不依赖运行时的 cwd）
# -----------------------------------------------------------------------------
# 编译期注入项目内容根目录 CONTENT_ROOT（不依赖 cwd）
set(CONTENT_ROOT_RAW "${CMAKE_SOURCE_DIR}")
string(REPLACE "\\" "/" CONTENT_ROOT_RAW "${CONTENT_ROOT_RAW}")
# 只传路径，不要包一层引号；C++ 里用字符串化宏得到字符串字面量
target_compile_definitions(DearEngine PRIVATE CONTENT_ROOT="${CONTENT_ROOT_RAW}")