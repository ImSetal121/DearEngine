//
// Created by ImSetal on 2026/2/12.
//

#include "Application.h"

#include <print>

namespace DA {
    // 递归：对单个实体及其所有子实体的组件调用 Start
    static void StartEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->Start(appstate);
        for (auto& child : entity->children)
            StartEntity(child.get(), appstate);
    }

    // 递归：对单个实体及其所有子实体的组件调用 Start
    static void IterateEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->Iterate(appstate);
        for (auto& child : entity->children)
            StartEntity(child.get(), appstate);
    }

    bool Application::Strat(void *appstate, DE::Scene *scene) {
        SetCurrentPlayingScene(scene);
        std::print("应用程序开始运行.\n");

        //遍历场景组件,调用组件Start方法.
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                StartEntity(entity.get(), appstate);
            }
        }

        return true;
    }

    bool Application::LogicIterate(void *appstate) {
        if (current_playing_scene) {
            for (auto& entity : current_playing_scene->root) {
                IterateEntity(entity.get(), appstate);
            }
        }
    }

    void Application::SetCurrentPlayingScene(DE::Scene* scene) {
        current_playing_scene = scene;
    }
} // DA