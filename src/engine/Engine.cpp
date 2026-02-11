//
// Created by ImSetal on 2026/2/11.
//

#include "Engine.h"

#include <filesystem>

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

namespace DE {
    long window_title_update_time = 0;

    void CheckCurrentPath() {
        printf("Current working directory: %s\n", std::filesystem::current_path().c_str());
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

            state->console_window->Draw();  // 控制台
            state->scene_tree_window->Draw();   // 场景树
            state->entity_component_window->Draw();   // GameObject组件
            state->scene_viewport_window->Draw();   // 场景视口

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
}
