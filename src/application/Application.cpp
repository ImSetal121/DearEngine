//
// Created by ImSetal on 2026/2/12.
//

#include "Application.h"

namespace DA {
    // 递归：对单个实体及其所有子实体的组件调用 Start
    static void InitEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->Init(appstate);
        for (auto& child : entity->children)
            InitEntity(child.get(), appstate);
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

    bool Application::Strat(void *appstate, DE::Scene *scene) {
        SetCurrentPlayingScene(scene);
        std::printf("应用程序开始运行.\n");

        //遍历场景组件,调用组件Start方法.
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                InitEntity(entity.get(), appstate);
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

    void Application::SetCurrentPlayingScene(DE::Scene* scene) {
        current_playing_scene = scene;
    }
} // DA