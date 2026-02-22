//
// Created by ImSetal on 2026/2/12.
//

#include "Application.h"

#include "../State.h"

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
    static void EndEntity(DE::Entity* entity, void* appstate, SDL_AppResult result) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->End();
        for (auto& child : entity->children)
            EndEntity(child.get(), appstate, result);
    }

    bool Application::Start(void *appstate, DE::Scene *scene) {
        SetCurrentPlayingScene(scene);
        std::printf("应用程序开始运行.\n");

        auto state = static_cast<AppState*>(appstate);

        //遍历场景组件,调用组件Start方法.
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                StartEntity(entity.get(), appstate);
            }
        }

        return true;
    }

    bool Application::Event(void *appstate, SDL_Event *event) {
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
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                RenderIterateEntity(entity.get(), appstate, nullptr);
            }
        }
        return true;
    }

    bool Application::End(void *appstate, SDL_AppResult result) {
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                EndEntity(entity.get(), appstate, result);
            }
        }
        return true;
    }

    void Application::SetCurrentPlayingScene(DE::Scene* scene) {
        current_playing_scene = scene;
    }
} // DA