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

#include "application/Application.h"

struct AppState {
    // 运行模式
    bool edit_mode = false;
    // SDL相关
    SDL_Window *editor_window = nullptr;
    SDL_Window *application_window = nullptr;
    SDL_GLContext gl_context = nullptr;
    //应用程序相关
    std::unique_ptr<DA::Application> application = nullptr;
    // 引擎相关
    bool application_is_running = false;
    Uint64 current_time_ns = 0;    // 计时
    double current_time = 0;
    Uint64 delta_time_ns = 0;    // 帧时间
    double delta_time = 0;
    std::vector<DE::IEngineWindow*> engine_windows;    // 引擎窗口
    DE::IEngineWindow *focused_engine_window = nullptr; // 当前获得焦点的引擎窗口（每帧由各窗口在 LogicIterate 中根据 ImGui 焦点更新）
    unsigned int default_program = 0;    // 场景视口
};

#endif //DEARENGINE_STATE_H