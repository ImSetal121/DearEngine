//
// Created by ImSetal on 2026/2/11.
//

#include "EngineEditor.h"

#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <sstream>

#include "imgui.h"
#include "../core/Log.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_dialog.h"
#include "window/ConsoleSubWindow.h"
#include "window/EntityComponentSubWindow.h"
#include "window/SceneTreeSubWindow.h"
#include "window/SceneViewportSubWindow.h"
#include "../../State.h"
#include "../../application/Application.h"
#include "../core/component/CameraComponent.h"
#include "../core/component/DirLightComponent.h"
#include "../core/component/TestCubeComponent.h"
#include "../core/component/TransformComponent.h"
#include "glad/glad.h"
#include "SDL3/SDL_storage.h"
#include "../util/Path.h"
#include "window/AssetManagerSubWindow.h"
#include "yaml-cpp/yaml.h"
#include "../core/serialization/ReflectYaml.h"

namespace DE {
    // 文件对话框结果（可能从其他线程回调，主线程在 LogicIterate 中处理）
    static std::mutex s_fileDialogMutex;
    static std::string s_pendingPath;
    enum class PendingFileAction { None, Save, Open };
    static PendingFileAction s_pendingAction = PendingFileAction::None;

    static void SDLCALL OnSaveFileDialog(void *userdata, const char *const *filelist, int filter) {
        (void)userdata;
        (void)filter;
        std::lock_guard<std::mutex> lock(s_fileDialogMutex);
        s_pendingAction = PendingFileAction::None;
        if (filelist && *filelist) {
            s_pendingPath = *filelist;
            s_pendingAction = PendingFileAction::Save;
        } else if (filelist == nullptr) {
            Log::Warning("保存场景: 文件对话框错误");
        }
    }

    static void SDLCALL OnOpenFileDialog(void *userdata, const char *const *filelist, int filter) {
        (void)userdata;
        (void)filter;
        std::lock_guard<std::mutex> lock(s_fileDialogMutex);
        s_pendingAction = PendingFileAction::None;
        if (filelist && *filelist) {
            s_pendingPath = *filelist;
            s_pendingAction = PendingFileAction::Open;
        } else if (filelist == nullptr) {
            Log::Warning("打开场景: 文件对话框错误");
        }
    }

    //正在编辑的场景
    std::unique_ptr<Scene> editing_scene = nullptr;
    //当前选中的实体
    Entity* selected_entity = nullptr;

    void ProcessPendingFileAction() {
        std::string path;
        PendingFileAction action = PendingFileAction::None;
        {
            std::lock_guard<std::mutex> lock(s_fileDialogMutex);
            if (s_pendingAction == PendingFileAction::None)
                return;
            path = s_pendingPath;
            action = s_pendingAction;
            s_pendingAction = PendingFileAction::None;
            s_pendingPath.clear();
        }
        if (action == PendingFileAction::Save && editing_scene && !path.empty()) {
            editing_scene->save_path = path;
            editing_scene->Save();
            editing_scene->name = std::filesystem::path(path).stem().string();
            Log::Info("场景已保存: " + path);
        } else if (action == PendingFileAction::Open && !path.empty()) {
            auto new_scene = std::make_unique<Scene>();
            new_scene->save_path = path;
            new_scene->Load();
            selected_entity = nullptr;
            editing_scene = std::move(new_scene);
            Log::Info("场景已打开: " + path);
        }
    }

    // 递归：对单个实体及其所有子实体的组件调用 EditorStart
    static void EditorStartEntity(DE::Entity* entity, void* appstate) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->EditorStart(appstate);
        for (auto& child : entity->children)
            EditorStartEntity(child.get(), appstate);
    }

    // 示例状态
    bool show_demo_window = false;
    long window_title_update_time = 0;

    bool EngineEditor::Init(void *appstate, int argc, char *argv[]) {
        auto *state = static_cast<AppState *>(appstate);

        // 欢迎语
        DE::Log::Info("Welcome to Dear Engine.");
        // 检查工作目录
        CheckCurrentPath();
        // 初始化state窗口
        state->editor_subwindows = {
            new ConsoleSubWindow(),
            new SceneTreeSubWindow(),
            new EntityComponentSubWindow(),
            new SceneViewportSubWindow(),
            new AssetManagerSubWindow()
        };
        for (IEditorSubWindow* window : state->editor_subwindows)
            window->Init(appstate);

        {
            // 测试场景
            auto test_scene = std::make_unique<Scene>();
            auto test_entity = std::make_unique<Entity>();
            test_entity->name = "entity";
            auto test_children = std::make_unique<Entity>();
            test_children->name = "children";
            auto test_entity_1 = std::make_unique<Entity>();
            test_entity_1->name = "entity_1";
            auto camera_entity = std::make_unique<Entity>();
            camera_entity->name = "camera";
            auto dir_light_entity = std::make_unique<Entity>();
            dir_light_entity->name = "dir_light";

            test_entity->AddComponent<TestCubeComponent>();
            test_entity->AddComponent<TransformComponent>();
            camera_entity->AddComponent<CameraComponent>();
            test_scene->main_camera = camera_entity->GetComponent<CameraComponent>();
            camera_entity->AddComponent<TransformComponent>();
            camera_entity->GetComponent<TransformComponent>()->position = glm::vec3(-5.0f, 0.0f, 0.0f);
            dir_light_entity->AddComponent<TransformComponent>();
            dir_light_entity->AddComponent<DirLightComponent>();
            // {
            //     auto trans = camera_entity->GetComponent<TransformComponent>();
            //     if (trans) {
            //         auto trans_t = DE::Reflect::GetByName("TransformComponent");
            //         for (auto v : trans_t.member_vars()) {
            //             Log::Info("menber:" + v.name());

            //         }

            //         glm::vec3 pos = trans_t.GetMemberVar("position").GetValue<glm::vec3>(*trans);
            //         Log::Info("position:" + std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z));

            //         trans_t.GetMemberVar("position").SetValue(*trans, glm::vec3(-3.0f, 0.0f, 0.0f));

            //         // 测试反射序列化为 YAML
            //         YAML::Node yaml = Reflect::SerializeReflectedToYaml(trans, "TransformComponent");
            //         if (yaml.IsMap()) {
            //             std::stringstream ss;
            //             ss << yaml;
            //             Log::Info("[TransformComponent 序列化] " + ss.str());
            //             std::string path = GetEngineAssetsPath() + "transform_test.yaml";
            //             if (Reflect::SaveReflectedToYamlFile(trans, "TransformComponent", path))
            //                 Log::Info("[TransformComponent 已写入] " + path);
            //         }

            //         // 测试从 transform.yaml 反序列化到当前组件
            //         std::string loadPath = GetEngineAssetsPath() + "transform.yaml";
            //         if (Reflect::LoadReflectedFromYamlFile(trans, "TransformComponent", loadPath)) {
            //             Log::Info("[TransformComponent 反序列化] 已从 " + loadPath + " 加载");
            //             glm::vec3 p = trans_t.GetMemberVar("position").GetValue<glm::vec3>(*trans);
            //             Log::Info("[TransformComponent 反序列化后] position: " +
            //                 std::to_string(p.x) + ", " + std::to_string(p.y) + ", " + std::to_string(p.z));
            //         } else {
            //             Log::Warning("[TransformComponent 反序列化] 加载失败: " + loadPath);
            //         }
            //     }
            // }
            test_entity->children.push_back(std::move(test_children));
            test_scene->root.push_back(std::move(test_entity));
            test_scene->root.push_back(std::move(test_entity_1));
            test_scene->root.push_back(std::move(camera_entity));
            test_scene->root.push_back(std::move(dir_light_entity));

            editing_scene = std::move(test_scene);

            for (auto& entity : editing_scene->root) {
                EditorStartEntity(entity.get(), appstate);
            }
        }

        // s_pendingPath = "/Users/imsetal/AllProjects/Project/DearEngine/src/application/assets/Untitled.scene";
        // s_pendingAction = PendingFileAction::Open;

        return true;
    }

    bool EngineEditor::Event(void *appstate, SDL_Event *event) {
        return true;
    }

    void StartPreviewApplication(AppState * state) {
        state->application = std::make_unique<DA::Application>();
        state->application->Start(state, editing_scene.get(), 0, {});
        state->application_is_running = true;
    }

    void StopPreviewApplication(AppState * state) {
        state->application_is_running = false;
    }

    bool EngineEditor::LogicIterate(void *appstate) {
        auto *state = static_cast<AppState *>(appstate);
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        if ((long)(state->current_time/1.0) != window_title_update_time) {
            std::string new_title = "Dear Engine Editor <OpenGL> [FPS:" + std::format("{:.2f}", io.Framerate)+"]";
            SDL_SetWindowTitle(state->editor_window, new_title.c_str());
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
            ProcessPendingFileAction();
            // 1. 最上方固定菜单栏
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("文件"))
                {
                    if (ImGui::MenuItem("保存场景")) {
                        if (!editing_scene) {
                            Log::Warning("没有正在编辑的场景，无法保存");
                        } else if (!editing_scene->save_path.empty()) {
                            editing_scene->Save();
                            Log::Info("场景已保存: " + editing_scene->save_path);
                        } else {
                            static const SDL_DialogFileFilter filters[] = {
                                { "场景文件 (*.scene)", "scene" },
                                { "All files", "*" }
                            };
                            std::string defaultSavePath = GetApplicationAssetsPath() + "Untitled.scene";
                            SDL_ShowSaveFileDialog(OnSaveFileDialog, nullptr, state->editor_window,
                                filters, 2, defaultSavePath.c_str());
                        }
                    }
                    if (ImGui::MenuItem("另存为场景")) {
                        if (!editing_scene) {
                            Log::Warning("没有正在编辑的场景，无法另存为");
                        } else {
                            static const SDL_DialogFileFilter filters[] = {
                                { "场景文件 (*.scene)", "scene" },
                                { "All files", "*" }
                            };
                            std::string defaultSavePath = editing_scene->save_path.empty()
                                ? (GetApplicationAssetsPath() + "Untitled.scene")
                                : editing_scene->save_path;
                            SDL_ShowSaveFileDialog(OnSaveFileDialog, nullptr, state->editor_window,
                                filters, 2, defaultSavePath.c_str());
                        }
                    }
                    if (ImGui::MenuItem("打开场景")) {
                        static const SDL_DialogFileFilter filters[] = {
                            { "场景文件 (*.yaml)", "yaml;yml" },
                            { "All files", "*" }
                        };
                        std::string defaultDir = GetApplicationAssetsPath();
                        if (!defaultDir.empty() && defaultDir.back() == '/')
                            defaultDir.pop_back();
                        SDL_ShowOpenFileDialog(OnOpenFileDialog, nullptr, state->editor_window,
                            filters, 2, defaultDir.c_str(), false);
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("关闭")) { return SDL_APP_SUCCESS; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("窗口"))
                {
                    for (IEditorSubWindow* window : state->editor_subwindows)
                        ImGui::Checkbox(window->Title(), &window->open);
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
                    if (state->application_is_running) StopPreviewApplication(state);
                    else StartPreviewApplication(state);
                }
                ImGui::SameLine();
                ImGui::EndMenuBar();
            }
            // 2. 下方是 DockSpace，其它窗口停靠到这里
            ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::End();

            for (IEditorSubWindow* window : state->editor_subwindows)
                window->LogicIterate(state);

            // 可选. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

        }

        if (GetEditingScene() == nullptr) {
            DE::Log::Warning("没有正在编辑的场景.");
        }

        return true;
    }

    bool EngineEditor::RenderIterate(void *appstate) {
        auto *state = static_cast<AppState *>(appstate);

        SDL_GL_MakeCurrent(state->editor_window, state->gl_context);

        // 引擎绘制
        for (IEditorSubWindow* window : state->editor_subwindows)
            window->RenderIterate(appstate);

        return true;
    }

    bool EngineEditor::Quit(void *appstate, SDL_AppResult result) {
        auto *state = static_cast<AppState *>(appstate);

        selected_entity = nullptr;

        for (IEditorSubWindow* window : state->editor_subwindows)
            window->Quit();

        return true;
    }

    Scene* EngineEditor::GetEditingScene() {
        return editing_scene ? editing_scene.get() : nullptr;
    }

    Entity * EngineEditor::GetSelectedEntity() {
        return selected_entity;
    }

    void EngineEditor::SetSelectedEntity(Entity *entity) {
        selected_entity = entity;
    }
}
