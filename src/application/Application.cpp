//
// Created by ImSetal on 2026/2/12.
//

#include "Application.h"

#include <format>

#include "../State.h"
#include "../engine/core/Log.h"
#include "../engine/util/GLUtil.h"
#include "glad/glad.h"
#include "SDL3/SDL_oldnames.h"

namespace DA {

    long window_title_update_time = 0;

    // 递归：对单个实体及其所有子实体的组件调用 Start
    static void StartEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->Start(appstate);
        for (auto& child : entity->children)
            StartEntity(child.get(), appstate);
    }

    // 递归：对单个实体及其所有子实体的组件调用 Event
    static void EventEntity(DE::Entity* entity, void* appstate, SDL_Event* event) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->Event();
        for (auto& child : entity->children)
            EventEntity(child.get(), appstate, event);
    }

    // 递归：对单个实体及其所有子实体的组件调用 LogicIterate
    static void LogicIterateEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->LogicIterate(appstate);
        for (auto& child : entity->children)
            LogicIterateEntity(child.get(), appstate);
    }

    // 递归：对单个实体及其所有子实体的组件调用 RenderIterate
    static void RenderIterateEntity(DE::Entity* entity, void* appstate, DE::RenderContext* render_context) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->RenderIterate(appstate, render_context);
        for (auto& child : entity->children)
            RenderIterateEntity(child.get(), appstate, render_context);
    }

    // 递归：对单个实体及其所有子实体的组件调用 End
    static void EndEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->End();
        for (auto& child : entity->children)
            EndEntity(child.get(), appstate);
    }

    bool Application::Start(void *appstate, DE::Scene *scene, int argc, char *argv[]) {
        SetCurrentPlayingScene(scene);
        std::printf("应用程序开始运行.\n");
        auto state = static_cast<AppState*>(appstate);

        // 显示程序窗口
        SDL_ShowWindow(state->application_window);

        //遍历场景组件,调用组件Start方法.
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                StartEntity(entity.get(), appstate);
            }
        }

        return true;
    }

    bool Application::Event(void *appstate, SDL_Event *event) {
        auto state = static_cast<AppState*>(appstate);
        if (state->application_window && event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(state->application_window)) {
            state->application_is_running = false;
            return false;
        }

        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                EventEntity(entity.get(), appstate, event);
            }
        }
        return true;
    }

    bool Application::LogicIterate(void *appstate) {
        auto state = static_cast<AppState*>(appstate);

        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                LogicIterateEntity(entity.get(), appstate);
            }
        }

        if ((long)(state->current_time/1.0) != window_title_update_time) {
            std::string new_title = state->application_name + " <OpenGL> [FPS:" + std::format("{:.2f}", 1.0/state->delta_time)+"]";
            SDL_SetWindowTitle(state->application_window, new_title.c_str());
            window_title_update_time = (long)(state->current_time/1.0);
        }

        return true;
    }

    bool Application::RenderIterate(void *appstate) {
        auto state = static_cast<AppState*>(appstate);

        SDL_GL_MakeCurrent(state->application_window, state->gl_context);

        int w, h;
        SDL_GetWindowSizeInPixels(state->application_window, &w, &h);

        glViewport(0, 0, w, h);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(state->default_program);

        if (current_playing_scene && current_playing_scene->main_camera->camera) {
            DE::RenderContext* render_context = new DE::RenderContext;
            render_context->camera = current_playing_scene->main_camera->camera;
            render_context->program = &state->default_program;
            render_context->screenWidth = &w;
            render_context->screenHeight = &h;

            glClearColor(render_context->camera->clear_color.x, render_context->camera->clear_color.y, render_context->camera->clear_color.z, render_context->camera->clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (auto& entity : current_playing_scene->root) {
                RenderIterateEntity(entity.get(), appstate, render_context);
            }

            delete render_context;
        }

        SDL_GL_SwapWindow(state->application_window);

        return true;
    }

    bool Application::End(void *appstate) {
        auto state = static_cast<AppState*>(appstate);

        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                EndEntity(entity.get(), appstate);
            }
        }

        SDL_HideWindow(state->application_window);

        return true;
    }

    void Application::SetCurrentPlayingScene(DE::Scene* scene) {
        current_playing_scene = scene;
    }
} // DA