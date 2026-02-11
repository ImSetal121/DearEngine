# 编译着色器
# 引入 shader 编译宏（glslc → spv，spirv-cross → msl）
include(${CMAKE_SOURCE_DIR}/cmake/compile_shader.cmake)

# 递归收集 src 下所有 glsl 文件，编译生成同名 .spv 和 .msl
file(GLOB_RECURSE GLSL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)
message(STATUS "[Shaders] Found ${GLSL_FILES} glsl file(s)")
list(LENGTH GLSL_FILES GLSL_COUNT)
if(GLSL_COUNT GREATER 0)
    message(STATUS "[Shaders] List: ${GLSL_FILES}")
endif()
set(SHADER_OUTPUTS "")
foreach(glsl_file ${GLSL_FILES})
    file(RELATIVE_PATH rel_glsl "${CMAKE_CURRENT_SOURCE_DIR}" "${glsl_file}")
    get_filename_component(shader_dir "${rel_glsl}" DIRECTORY)
    get_filename_component(name_we "${rel_glsl}" NAME_WE)
    set(spv_path "${shader_dir}/${name_we}.spv")
    set(msl_path "${shader_dir}/${name_we}.msl")
    message(STATUS "[Shaders] ${rel_glsl} -> ${spv_path}, ${msl_path}")
    compile_shader_to_spv("${rel_glsl}" "${spv_path}")
    compile_shader_to_msl("${spv_path}" "${msl_path}")
    list(APPEND SHADER_OUTPUTS "${CMAKE_CURRENT_SOURCE_DIR}/${spv_path}" "${CMAKE_CURRENT_SOURCE_DIR}/${msl_path}")
endforeach()
add_custom_target(compile_all_shaders DEPENDS ${SHADER_OUTPUTS})

# DearEngine 可执行文件（源、链接、include）

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(ALL_FILES ${SRC_FILES} ${HEADER_FILES})

add_executable(DearEngine
        ${ALL_FILES}
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

add_dependencies(DearEngine compile_all_shaders)

# Link to the library.
target_link_libraries(DearEngine PRIVATE
        SDL3::SDL3
)

# Add include directories.
target_include_directories(DearEngine PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)