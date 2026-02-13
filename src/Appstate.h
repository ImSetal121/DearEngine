//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_APPSTATE_H
#define DEARENGINE_APPSTATE_H

#include "engine/window/ConsoleWindow.h"
#include "engine/window/EntityComponentWindow.h"
#include "engine/window/SceneTreeWindow.h"
#include "engine/window/SceneViewportWindow.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_video.h"

struct AppState {
    // SDL相关
    SDL_Window *engine_window = nullptr;
    SDL_GPUDevice *gpu_device = nullptr;
    // 引擎相关
    bool application_is_running = false;
    Uint64 current_time_ns = 0;
    Uint64 delta_time_ns = 0;
    double current_time = 0;
    double delta_time = 0;
    // 引擎场景视口绘制
    SDL_GPUTexture* scene_viewport_texture = nullptr;
    SDL_GPUShader* scene_vertex_shader = nullptr;
    SDL_GPUShader* scene_fragment_shader = nullptr;
    SDL_GPUGraphicsPipeline* scene_triangle_pipeline = nullptr;
    int scene_viewport_texture_width = 1280;
    int scene_viewport_texture_height = 720;
    // 引擎窗口
    ConsoleWindow *console_window = nullptr;
    SceneTreeWindow *scene_tree_window = nullptr;
    EntityComponentWindow *entity_component_window = nullptr;
    SceneViewportWindow *scene_viewport_window = nullptr;
};

#endif //DEARENGINE_APPSTATE_H