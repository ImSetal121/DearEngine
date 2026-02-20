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
    SDL_GLContext gl_context = nullptr; //opengl上下文
    // 引擎相关
    bool application_is_running = false;
    // 计时
    Uint64 current_time_ns = 0;
    double current_time = 0;
    // 帧时间
    Uint64 delta_time_ns = 0;
    double delta_time = 0;
    // 引擎窗口
    std::vector<DE::IEngineWindow*> engine_windows;
    DE::IEngineWindow* focused_engine_window = nullptr; // 当前获得焦点的引擎窗口（每帧由各窗口在 LogicIterate 中根据 ImGui 焦点更新）
    // 场景视口
    unsigned int* scene_program = nullptr;
};

#endif //DEARENGINE_STATE_H