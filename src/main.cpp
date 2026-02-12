#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "Appstate.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "engine/Engine.h"
#include "engine/core/Log.h"
#include "engine/util/AboutGPU.h"
#include "engine/window/ConsoleWindow.h"
#include "engine/window/EntityComponentWindow.h"
#include "engine/window/SceneTreeWindow.h"
#include "engine/window/SceneViewportWindow.h"

std::string GetEngineAssetsPath() {
    return "./src/engine/assets/";
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    for (int i = 0; i < SDL_GetNumGPUDrivers(); i++) {
        std::cout << SDL_GetGPUDriver(i) << std::endl;
    }

    // 初始化AppState
    auto *state = new AppState();

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // 创建 SDL 窗口与图形上下文
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    state->engine_window = SDL_CreateWindow("Dear Engine", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (state->engine_window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(state->engine_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(state->engine_window);

    // 创建 GPU 设备
    state->gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB,true,nullptr);
    if (state->gpu_device == nullptr)
    {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // 将窗口交由 GPU 设备使用
    if (!SDL_ClaimWindowForGPUDevice(state->gpu_device, state->engine_window))
    {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetGPUSwapchainParameters(state->gpu_device, state->engine_window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // 设置 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // ImGui 布局：若用户尚未有 imgui.ini，则从默认模板复制
    const char* user_ini = "imgui.ini";
    std::string ini_path = GetEngineAssetsPath() + "config/imgui_default.ini";
    const char* default_ini = ini_path.c_str();
    io.IniFilename = user_ini;

    std::ifstream check(user_ini);
    if (!check.good()) {
        std::ifstream src(default_ini, std::ios::binary);
        std::ofstream dst(user_ini, std::ios::binary);
        if (src && dst)
            dst << src.rdbuf();
    }
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 启用键盘控制
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // 启用手柄控制
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // 启用停靠
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // 启用多视口 / 平台窗口

    // 设置 Dear ImGui 样式
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // 设置缩放
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // 固定样式缩放（在支持动态样式缩放前，修改后需重置 Style 并再次调用）
    style.FontScaleDpi = main_scale;        // 设置初始字体缩放（io.ConfigDpiScaleFonts=true 时可不设，此处保留作文档说明）
    io.ConfigDpiScaleFonts = true;          // [实验] 显示器 DPI 变化时在 Begin() 中自动覆盖 style.FontScaleDpi，目前仅缩放字体，不缩放尺寸/内边距
    io.ConfigDpiScaleViewports = true;      // [实验] 显示器 DPI 变化时缩放 Dear ImGui 与平台窗口

    // 启用视口时微调 WindowRounding/WindowBg，使平台窗口与普通窗口外观一致
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 设置平台/渲染器后端
    ImGui_ImplSDL3_InitForSDLGPU(state->engine_window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = state->gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(state->gpu_device, state->engine_window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // 仅在多视口模式下使用
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // 仅在多视口模式下使用
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // 加载字体
    std::string font_path_str = GetEngineAssetsPath() + "ttf/HarmonyOS_Sans_SC/HarmonyOS_Sans_SC_Regular.ttf";
    const char* font_path_chinese = font_path_str.c_str();
    if (font_path_chinese)
        io.Fonts->AddFontFromFileTTF(font_path_chinese, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    // 引擎Init
    DE::Engine::Init(state, argc, argv);

    // 返回AppState指针
    *appstate = state;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    auto *state = static_cast<AppState *>(appstate);

    // 轮询并处理事件（输入、窗口缩放等）
    // 可通过 io.WantCaptureMouse、io.WantCaptureKeyboard 判断 dear imgui 是否要接管输入。
    // - io.WantCaptureMouse 为 true 时，不要将鼠标输入发给主应用，或清空/覆盖你的鼠标数据副本。
    // - io.WantCaptureKeyboard 为 true 时，不要将键盘输入发给主应用，或清空/覆盖你的键盘数据副本。
    // 通常可始终把所有输入传给 dear imgui，再根据这两个标志决定是否对主应用隐藏。
    ImGui_ImplSDL3_ProcessEvent(event);
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(state->engine_window))
        return SDL_APP_SUCCESS;

    if (DE::Engine::Event(appstate, event))
        return SDL_APP_CONTINUE;
    else
        return SDL_APP_FAILURE;

}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    if (SDL_GetWindowFlags(state->engine_window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;;
    }

    // 开始 Dear ImGui 本帧
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 引擎逻辑tick
    if (!DE::Engine::LogicIterate(appstate))
        return SDL_APP_FAILURE;

    // 渲染
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(state->gpu_device); // 获取 GPU 命令缓冲

    // 引擎渲染Tick
    if (!DE::Engine::RenderIterate(appstate, command_buffer))
        return SDL_APP_FAILURE;

    SDL_GPUTexture* swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state->engine_window, &swapchain_texture, nullptr, nullptr); // 获取交换链纹理

    if (swapchain_texture != nullptr && !is_minimized)
    {
        // 必须调用：ImGui_ImplSDLGPU3_PrepareDrawData() 用于上传顶点/索引缓冲
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        // 配置并开始一次渲染通道
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        SDL_GPUColorTargetInfo target_info = {};
        target_info.texture = swapchain_texture;
        target_info.clear_color = SDL_FColor { clear_color.x, clear_color.y, clear_color.z, clear_color.w };
        target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        target_info.store_op = SDL_GPU_STOREOP_STORE;
        target_info.mip_level = 0;
        target_info.layer_or_depth_plane = 0;
        target_info.cycle = false;
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

        // 渲染 ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

        SDL_EndGPURenderPass(render_pass);
    }

    // 更新并渲染额外的平台窗口
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    // 提交命令缓冲
    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *state = static_cast<AppState *>(appstate);
    // 清理
    SDL_WaitForGPUIdle(state->gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    DE::Engine::Quit(appstate, result);

    SDL_ReleaseWindowFromGPUDevice(state->gpu_device, state->engine_window);
    SDL_DestroyGPUDevice(state->gpu_device);
    SDL_DestroyWindow(state->engine_window);
    SDL_Quit();
}
