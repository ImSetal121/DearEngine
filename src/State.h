//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_STATE_H
#define DEARENGINE_STATE_H

#include "SDL3/SDL_video.h"

#include "application/Application.h"
#include "engine/editor/window/IEditorSubWindow.h"

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
    std::vector<DE::IEditorSubWindow*> editor_subwindows;    // 引擎窗口
    DE::IEditorSubWindow *focused_editor_subwindow = nullptr; // 当前获得焦点的引擎窗口（每帧由各窗口在 LogicIterate 中根据 ImGui 焦点更新）
    unsigned int default_program = 0;    // 场景视口
};

#endif //DEARENGINE_STATE_H