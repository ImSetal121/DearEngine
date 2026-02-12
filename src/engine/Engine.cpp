//
// Created by ImSetal on 2026/2/11.
//

#include "Engine.h"

#include <filesystem>
#include <memory>

#include "imgui.h"
#include "core/Log.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_storage.h"
#include "SDL3/SDL_timer.h"
#include "util/AboutGPU.h"
#include "window/ConsoleWindow.h"
#include "window/EntityComponentWindow.h"
#include "window/SceneTreeWindow.h"
#include "window/SceneViewportWindow.h"
#include "../Appstate.h"
#include "core/component/TestComponent.h"

namespace DE {
    //正在编辑的场景
    std::unique_ptr<Scene> editing_scene = nullptr;
    //当前选中的实体
    Entity* selected_entity = nullptr;

    // 示例状态
    bool show_demo_window = false;
    long window_title_update_time = 0;

    void CheckCurrentPath() {
        Log::Info(std::string("当前工作路径: ") + std::filesystem::current_path().string()+"\n");
    }

    std::string GetEngineAssetsPath() {
        return "./src/engine/assets/";
    }

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
        // 为场景视口创建绘制纹理
        SDL_GPUTextureCreateInfo texture_info = {};
        texture_info.type = SDL_GPU_TEXTURETYPE_2D;              // 二维纹理
        texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;  // 每像素 RGBA 8 位，归一化 [0,1]
        texture_info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;  // 可作渲染目标且可被采样（如 ImGui 显示）
        texture_info.width = (Uint32)state->scene_viewport_texture_width;   // 纹理宽度（像素）
        texture_info.height = (Uint32)state->scene_viewport_texture_height;  // 纹理高度（像素）
        texture_info.layer_count_or_depth = 1;   // 2D 纹理层数/3D 深度，此处为 1
        texture_info.num_levels = 1;             //  mip 级数，1 表示不生成 mip
        texture_info.sample_count = SDL_GPU_SAMPLECOUNT_1;  // 每像素 1 采样（无多重采样）
        texture_info.props = 0;                 // 扩展属性，0 表示无
        state->scene_viewport_texture = SDL_CreateGPUTexture(state->gpu_device, &texture_info);
        if (state->scene_viewport_texture)
            state->scene_viewport_window->SetViewportTexture(state->scene_viewport_texture, state->scene_viewport_texture_width, state->scene_viewport_texture_height);

        SDL_Storage* storage = SDL_OpenFileStorage(GetEngineAssetsPath().c_str());
        if (storage) {
            while (!SDL_StorageReady(storage)) SDL_Delay(1);
            state->scene_vertex_shader   = LoadShader(state->gpu_device, "shader/default_vert", storage, SDL_GPU_SHADERSTAGE_VERTEX);
            state->scene_fragment_shader = LoadShader(state->gpu_device, "shader/default_frag", storage, SDL_GPU_SHADERSTAGE_FRAGMENT);
            SDL_CloseStorage(storage);
            if (state->scene_vertex_shader && state->scene_fragment_shader) {
                state->scene_triangle_pipeline = CreatePipeline(state->gpu_device, state->scene_vertex_shader, state->scene_fragment_shader);
            }
        }

        {
            // 测试场景
            auto test_scene = std::make_unique<Scene>();
            auto test_entity = std::make_unique<Entity>();
            test_entity->name = "entity";
            auto test_children = std::make_unique<Entity>();
            test_children->name = "children";
            auto test_entity_1 = std::make_unique<Entity>();
            test_entity_1->name = "entity_1";

            test_entity->children_.push_back(std::move(test_children));
            test_entity->AddComponent<TestComponent>();
            test_scene->root_.push_back(std::move(test_entity));
            test_scene->root_.push_back(std::move(test_entity_1));

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
        Uint64 delta_time_ns = current_time_ns - state->current_time_ns;
        state->current_time_ns = current_time_ns;
        double current_time = current_time_ns / 1000000000.0;
        double delta_time = delta_time_ns / 1000000000.0;

        if ((long)(current_time/1.0) != window_title_update_time) {
            std::string new_title = "Dear Engine  [FPS:" + std::to_string(io.Framerate)+"]";
            SDL_SetWindowTitle(state->engine_window, new_title.c_str());
            window_title_update_time = (long)(current_time/1.0);
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
                if (ImGui::MenuItem("运行")) {
                    DE::Log::Debug("开始运行.");
                }
                ImGui::SameLine();
                ImGui::EndMenuBar();
            }
            // 2. 下方是 DockSpace，其它窗口停靠到这里
            ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::End();

            state->console_window->Draw();  // 控制台
            state->scene_tree_window->Draw();   // 场景树
            state->entity_component_window->Draw();   // 实体组件
            state->scene_viewport_window->Draw();   // 场景视口

            // 可选. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

        }

        if (GetEditingScene() == nullptr) {
            DE::Log::Warning("没有正在编辑的场景.");
        }

        return true;
    }

    bool Engine::RenderIterate(void *appstate, SDL_GPUCommandBuffer* command_buffer) {
        auto *state = static_cast<AppState *>(appstate);

        // 引擎绘制
        if (state->scene_viewport_texture && state->scene_triangle_pipeline && state->scene_viewport_texture_width > 0 && state->scene_viewport_texture_height > 0) {
            SDL_GPUColorTargetInfo viewport_target = {};
            viewport_target.texture = state->scene_viewport_texture;
            viewport_target.clear_color = SDL_FColor{ 0.1f, 0.1f, 0.1f, 1.0f };
            viewport_target.load_op = SDL_GPU_LOADOP_CLEAR;
            viewport_target.store_op = SDL_GPU_STOREOP_STORE;
            viewport_target.mip_level = 0;
            viewport_target.layer_or_depth_plane = 0;
            viewport_target.cycle = false;
            SDL_GPURenderPass* rp = SDL_BeginGPURenderPass(command_buffer, &viewport_target, 1, nullptr);
            SDL_BindGPUGraphicsPipeline(rp, state->scene_triangle_pipeline);
            SDL_GPUViewport vp = {};
            vp.x = 0;
            vp.y = 0;
            vp.w = state->scene_viewport_texture_width;
            vp.h = state->scene_viewport_texture_height;
            vp.min_depth = 0.0f;
            vp.max_depth = 1.0f;
            SDL_SetGPUViewport(rp, &vp);
            SDL_DrawGPUPrimitives(rp, 3, 1, 0, 0);
            SDL_EndGPURenderPass(rp);
        }

        return true;
    }

    bool Engine::Quit(void *appstate, SDL_AppResult result) {
        auto *state = static_cast<AppState *>(appstate);

        if (state->scene_viewport_texture) {
            SDL_ReleaseGPUTexture(state->gpu_device, state->scene_viewport_texture);
            state->scene_viewport_texture = nullptr;
        }
        if (state->scene_triangle_pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(state->gpu_device, state->scene_triangle_pipeline);
            state->scene_triangle_pipeline = nullptr;
        }
        if (state->scene_vertex_shader) {
            SDL_ReleaseGPUShader(state->gpu_device, state->scene_vertex_shader);
            state->scene_vertex_shader = nullptr;
        }
        if (state->scene_fragment_shader) {
            SDL_ReleaseGPUShader(state->gpu_device, state->scene_fragment_shader);
            state->scene_fragment_shader = nullptr;
        }
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
