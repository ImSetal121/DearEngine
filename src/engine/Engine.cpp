//
// Created by ImSetal on 2026/2/11.
//

#include "Engine.h"

#include <filesystem>
#include <format>
#include <memory>

#include "imgui.h"
#include "core/Log.h"
#include "SDL3/SDL_timer.h"
#include "window/ConsoleWindow.h"
#include "window/EntityComponentWindow.h"
#include "window/SceneTreeWindow.h"
#include "window/SceneViewportWindow.h"
#include "../State.h"
#include "../application/Application.h"
#include "core/component/TestComponent.h"
#include "glad/glad.h"
#include "SDL3/SDL_storage.h"
#include "util/Path.h"

namespace DE {
    //正在编辑的场景
    std::unique_ptr<Scene> editing_scene = nullptr;
    //当前选中的实体
    Entity* selected_entity = nullptr;
    //预览应用程序
    std::unique_ptr<DA::Application> preview_application = nullptr;

    // 示例状态
    bool show_demo_window = false;
    long window_title_update_time = 0;

    bool Engine::Init(void *appstate, int argc, char *argv[]) {
        auto *state = static_cast<AppState *>(appstate);

        // 欢迎语
        DE::Log::Info("Welcome to Dear Engine.");
        // 检查工作目录
        CheckCurrentPath();
        // 初始化state窗口
        state->console_window = new ConsoleWindow();
        state->scene_tree_window = new SceneTreeWindow();
        state->entity_component_window = new EntityComponentWindow();
        state->scene_viewport_window = new SceneViewportWindow();
        state->scene_viewport_window->Init();

        {
            // 测试场景
            auto test_scene = std::make_unique<Scene>();
            auto test_entity = std::make_unique<Entity>();
            test_entity->name = "entity";
            auto test_children = std::make_unique<Entity>();
            test_children->name = "children";
            auto test_entity_1 = std::make_unique<Entity>();
            test_entity_1->name = "entity_1";

            test_entity->children.push_back(std::move(test_children));
            test_entity->AddComponent<TestComponent>();
            test_scene->root.push_back(std::move(test_entity));
            test_scene->root.push_back(std::move(test_entity_1));

            editing_scene = std::move(test_scene);
        }

        return true;
    }

    bool Engine::Event(void *appstate, SDL_Event *event) {
        return true;
    }

    bool Engine::LogicIterate(void *appstate) {
        auto *state = static_cast<AppState *>(appstate);
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        Uint64 current_time_ns = SDL_GetTicksNS();
        state->delta_time_ns = current_time_ns - state->current_time_ns;
        state->current_time_ns = current_time_ns;
        state->current_time = current_time_ns / 1000000000.0;
        state->delta_time = state->delta_time_ns / 1000000000.0;

        if ((long)(state->current_time/1.0) != window_title_update_time) {
            std::string new_title = "Dear Engine  [FPS:" + std::format("{:.2f}", io.Framerate)+"]";
            SDL_SetWindowTitle(state->engine_window, new_title.c_str());
            window_title_update_time = (long)(state->current_time/1.0);
        }

        {   // 引擎窗口绘制

            // 0. 创建主停靠窗口
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking
                | ImGuiWindowFlags_MenuBar;   // 需要菜单栏就加 MenuBar
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("MainDockHost", nullptr, host_flags);
            ImGui::PopStyleVar(3);
            // 1. 最上方固定菜单栏
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("文件"))
                {
                    if (ImGui::MenuItem("保存")) { /* ... */ }
                    ImGui::Separator();
                    if (ImGui::MenuItem("关闭")) { return SDL_APP_SUCCESS; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("窗口"))
                {
                    ImGui::Checkbox(state->console_window->Title(), &state->console_window->open);
                    ImGui::Checkbox(state->scene_tree_window->Title(), &state->scene_tree_window->open);
                    ImGui::Checkbox(state->entity_component_window->Title(), &state->entity_component_window->open);
                    ImGui::Checkbox(state->scene_viewport_window->Title(), &state->scene_viewport_window->open);
                    ImGui::Separator();
                    ImGui::Checkbox("ImGui演示窗口", &show_demo_window);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("帮助"))
                {
                    if (ImGui::MenuItem("关于Dear Engine")) { /* ... */ }
                    ImGui::EndMenu();
                }
                // 将运行按钮放在中间（约窗口宽度的中央）
                ImGui::SameLine((ImGui::GetWindowWidth() - 80.0f) * 0.5f);  // 80 为按钮大致宽度
                if (ImGui::MenuItem(state->application_is_running ? "停止" : "运行")) {
                    if (state->application_is_running) {
                        state->application_is_running = false;
                    } else {
                        preview_application = std::make_unique<DA::Application>();
                        preview_application->Strat(state, editing_scene.get());

                        state->application_is_running = true;
                    }
                }
                ImGui::SameLine();
                ImGui::EndMenuBar();
            }
            // 2. 下方是 DockSpace，其它窗口停靠到这里
            ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::End();

            state->console_window->LogicIterate();  // 控制台
            state->scene_tree_window->LogicIterate();   // 场景树
            state->entity_component_window->LogicIterate();   // 实体组件
            state->scene_viewport_window->LogicIterate();   // 场景视口

            // 可选. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

        }

        if (GetEditingScene() == nullptr) {
            DE::Log::Warning("没有正在编辑的场景.");
        }

        if (state->application_is_running && preview_application) {
            preview_application->LogicIterate(state);
        }

        return true;
    }

    bool Engine::RenderIterate(void *appstate) {
        auto *state = static_cast<AppState *>(appstate);

        // 引擎绘制
        state->scene_viewport_window->RenderIterate();

        return true;
    }

    bool Engine::Quit(void *appstate, SDL_AppResult result) {
        auto *state = static_cast<AppState *>(appstate);

        selected_entity = nullptr;

        state->scene_viewport_window->Quit();

        return true;
    }

    Scene* Engine::GetEditingScene() {
        return editing_scene ? editing_scene.get() : nullptr;
    }

    Entity * Engine::GetSelectedEntity() {
        return selected_entity;
    }

    void Engine::SetSelectedEntity(Entity *entity) {
        selected_entity = entity;
    }
}
