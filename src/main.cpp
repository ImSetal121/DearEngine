#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "engine/util/AboutGPU.h"
#include "engine/window/ConsoleWindow.h"
#include "engine/window/GameobjectComponentWindow.h"
#include "engine/window/SceneTreeWindow.h"
#include "engine/window/SceneViewportWindow.h"

// 示例状态
bool show_demo_window = false;

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
long window_title_update_time = 0;

struct AppState {
    // SDL相关
    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpu_device = nullptr;
    // 引擎相关
    Uint64 current_time_ns = 0;
    // 引擎场景视口绘制
    SDL_GPUTexture* scene_viewport_texture = nullptr;
    SDL_GPUShader* scene_vertex_shader = nullptr;
    SDL_GPUShader* scene_fragment_shader = nullptr;
    SDL_GPUGraphicsPipeline* scene_triangle_pipeline = nullptr;
    int scene_viewport_texture_width = 1280;
    int scene_viewport_texture_height = 720;
    // 引擎窗口
    ConsoleWindow *console_window = nullptr;
    SceneTreeWindow *scene_tree_window = nullptr;
    GameobjectComponentWindow *gameobject_component_window = nullptr;
    SceneViewportWindow *scene_viewport_window = nullptr;
};

void CheckCurrentPath() {
    printf("Current working directory: %s\n", std::filesystem::current_path().c_str());
}

std::string GetEngineAssetsPath() {
    return "./src/engine/assets/";
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
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
    state->window = SDL_CreateWindow("Dear Engine.", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (state->window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(state->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(state->window);

    // 创建 GPU 设备
    state->gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB,true,nullptr);
    if (state->gpu_device == nullptr)
    {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // 将窗口交由 GPU 设备使用
    if (!SDL_ClaimWindowForGPUDevice(state->gpu_device, state->window))
    {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetGPUSwapchainParameters(state->gpu_device, state->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

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
    ImGui_ImplSDL3_InitForSDLGPU(state->window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = state->gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(state->gpu_device, state->window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // 仅在多视口模式下使用
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // 仅在多视口模式下使用
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // 引擎部分
    {
        // 检查工作目录
        CheckCurrentPath();
        // 加载字体
        std::string font_path_str = GetEngineAssetsPath() + "ttf/HarmonyOS_Sans_SC/HarmonyOS_Sans_SC_Regular.ttf";
        const char* font_path_chinese = font_path_str.c_str();
        if (font_path_chinese)
            io.Fonts->AddFontFromFileTTF(font_path_chinese, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        // 初始化state窗口
        state->console_window = new ConsoleWindow();
        state->scene_tree_window = new SceneTreeWindow();
        state->gameobject_component_window = new GameobjectComponentWindow();
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

        // 返回AppState指针
        *appstate = state;
    }

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
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(state->window))
        return SDL_APP_SUCCESS;

    return SDL_APP_CONTINUE;

}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    if (SDL_GetWindowFlags(state->window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;;
    }

    // 开始 Dear ImGui 本帧
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

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
            ImGui::Checkbox(state->gameobject_component_window->Title(), &state->gameobject_component_window->open);
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
        ImGui::EndMenuBar();
    }

    // 2. 下方是 DockSpace，其它窗口停靠到这里
    ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();

    // 引擎部分
    {
        Uint64 current_time_ns = SDL_GetTicksNS();
        Uint64 delta_time_ns = current_time_ns - state->current_time_ns;
        state->current_time_ns = current_time_ns;
        double current_time = current_time_ns / 1000000000.0;
        double delta_time = delta_time_ns / 1000000000.0;

        if ((long)(current_time/0.2) != window_title_update_time) {
            std::string new_title = "Dear Engine.  FPS:" + std::to_string(io.Framerate);
            SDL_SetWindowTitle(state->window, new_title.c_str());
            window_title_update_time = (long)(current_time/0.2);
        }

        {   // 引擎窗口绘制

            state->console_window->Draw();  // 控制台
            state->scene_tree_window->Draw();   // 场景树
            state->gameobject_component_window->Draw();   // GameObject组件
            state->scene_viewport_window->Draw();   // 场景视口

        }
    }

    // 可选. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 渲染
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(state->gpu_device); // 获取 GPU 命令缓冲

    {
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
    }

    SDL_GPUTexture* swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, state->window, &swapchain_texture, nullptr, nullptr); // 获取交换链纹理

    if (swapchain_texture != nullptr && !is_minimized)
    {
        // 必须调用：ImGui_ImplSDLGPU3_PrepareDrawData() 用于上传顶点/索引缓冲
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

        // 配置并开始一次渲染通道
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

    {
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
    }

    SDL_ReleaseWindowFromGPUDevice(state->gpu_device, state->window);
    SDL_DestroyGPUDevice(state->gpu_device);
    SDL_DestroyWindow(state->window);
    SDL_Quit();
}
