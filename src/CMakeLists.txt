# 只负责 LearnCPP 可执行文件（源、链接、include）

add_executable(LearnCPP
        main.cpp
        # ImGui 核心
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        # SDL3 + SDL_GPU3 后端
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
)

# Link to the library.
target_link_libraries(LearnCPP PRIVATE
        SDL3::SDL3
)

# Add include directories.
target_include_directories(LearnCPP PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)