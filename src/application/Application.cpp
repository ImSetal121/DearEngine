//
// Created by ImSetal on 2026/2/12.
//

#include "Application.h"

#include "../State.h"
#include "../engine/core/Log.h"
#include "../engine/util/GLUtil.h"
#include "glad/glad.h"
#include "SDL3/SDL_oldnames.h"

namespace DA {

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

    bool Application::Start(void *appstate, DE::Scene *scene) {
        SetCurrentPlayingScene(scene);
        std::printf("应用程序开始运行.\n");
        auto state = static_cast<AppState*>(appstate);

        // 创建程序窗口
        const char *glsl_version = DE::SelectGLVersion();
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
        state->application_window = SDL_CreateWindow("Dear Application", (int)(854 * main_scale), (int)(480 * main_scale), window_flags);
        if (state->application_window == nullptr)
        {
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }
        SDL_GL_SetSwapInterval(1); // 开启垂直同步
        SDL_SetWindowPosition(state->application_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(state->application_window);

        // 创建OpenGL上下文
        SDL_GL_MakeCurrent(state->editor_window, state->editor_gl_context);
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, true);
        state->application_gl_context = SDL_GL_CreateContext(state->application_window);
        if (state->application_gl_context == nullptr) {
            printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
            return false;
        }

        // 加载glad
        gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

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
        }

        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                EventEntity(entity.get(), appstate, event);
            }
        }
        return true;
    }

    bool Application::LogicIterate(void *appstate) {
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                LogicIterateEntity(entity.get(), appstate);
            }
        }
        return true;
    }

    bool Application::RenderIterate(void *appstate) {
        auto state = static_cast<AppState*>(appstate);

        SDL_GL_MakeCurrent(state->application_window, state->application_gl_context);

        // 检查着色器程序
        GLint isLinked = 0;
        glGetProgramiv(state->default_program, GL_LINK_STATUS, &isLinked);
        printf("着色器链接状态: %s\n", isLinked ? "成功" : "失败");

        if (!isLinked) {
            char buf[512];
            glGetProgramInfoLog(state->default_program, sizeof(buf), nullptr, buf);
            printf("着色器错误: %s\n", buf);
        }


        int w, h;
        SDL_GetWindowSizeInPixels(state->application_window, &w, &h);

        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glUseProgram(state->default_program);

        if (current_playing_scene && current_playing_scene->main_camera->camera) {
            DE::RenderContext render_context;
            render_context.camera = current_playing_scene->main_camera->camera;
            render_context.program = &state->default_program;
            render_context.screenWidth = &w;
            render_context.screenHeight = &h;

            for (auto& entity : current_playing_scene->root) {
                RenderIterateEntity(entity.get(), appstate, &render_context);
            }
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
        if (state->application_window) {
            SDL_DestroyWindow(state->application_window);
            state->application_window = nullptr;
        }
        return true;
    }

    void Application::SetCurrentPlayingScene(DE::Scene* scene) {
        current_playing_scene = scene;
    }
} // DA