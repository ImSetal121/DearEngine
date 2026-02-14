//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_STATE_H
#define DEARENGINE_STATE_H

#include "engine/window/ConsoleWindow.h"
#include "engine/window/EntityComponentWindow.h"
#include "engine/window/SceneTreeWindow.h"
#include "engine/window/SceneViewportWindow.h"
#include "SDL3/SDL_video.h"

struct AppState {
    // SDL相关
    SDL_Window *engine_window = nullptr;

    // SDL_GPUDevice *gpu_device = nullptr;
    SDL_GLContext gl_context = nullptr; //opengl上下文

    // 引擎相关
    bool application_is_running = false;
    Uint64 current_time_ns = 0;
    Uint64 delta_time_ns = 0;
    double current_time = 0;
    double delta_time = 0;
    // 引擎场景视口绘制

    // SDL_GPUTexture* scene_viewport_texture = nullptr;
    // SDL_GPUShader* scene_vertex_shader = nullptr;
    // SDL_GPUShader* scene_fragment_shader = nullptr;
    // SDL_GPUGraphicsPipeline* scene_triangle_pipeline = nullptr;
    unsigned int scene_viewport_fbo = 0;        // GL FBO
    unsigned int scene_viewport_texture = 0;    // GL 纹理 id，供 ImGui::Image 使用
    unsigned int scene_program = 0;             // 场景用着色器程序（如画三角形）
    unsigned int scene_vao = 0;                 // 场景用 VAO（Core 下 glDrawArrays 必须绑定 VAO）

    int scene_viewport_texture_width = 1280;
    int scene_viewport_texture_height = 720;
    // 引擎窗口
    ConsoleWindow *console_window = nullptr;
    SceneTreeWindow *scene_tree_window = nullptr;
    EntityComponentWindow *entity_component_window = nullptr;
    SceneViewportWindow *scene_viewport_window = nullptr;
};

#endif //DEARENGINE_STATE_H