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
        ${OPENGL_LIBRARIES}
)

target_include_directories(DearEngine PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${OPENGL_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)