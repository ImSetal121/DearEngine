cmake_minimum_required(VERSION 4.0)
project(LearnCPP)

set(CMAKE_CXX_STANDARD 26)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodule/imgui)

# This assumes the SDL source is available in submodule/SDL
add_subdirectory(submodule/SDL EXCLUDE_FROM_ALL)

add_executable(LearnCPP
        main.cpp
        # ImGui 核心
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        # SDL3 + SDL_Renderer 后端
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
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
