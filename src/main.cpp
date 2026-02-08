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
#include "window/ConsoleWindow.h"
#include "window/GameobjectComponentWindow.h"
#include "window/SceneTreeWindow.h"
#include "window/SceneViewportWindow.h"

// 本示例状态
bool show_example_window = false;
bool show_demo_window = false;
bool show_another_window = false;
bool show_another_window2 = false;

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
long window_title_update_time = 0;

struct AppState {
    // SDL相关
    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpu_device = nullptr;
    // 引擎相关
    Uint64 current_time_ns = 0;
    // 引擎窗口
    ConsoleWindow *console_window = nullptr;
    SceneTreeWindow *scene_tree_window = nullptr;
    GameobjectComponentWindow *gameobject_component_window = nullptr;
    SceneViewportWindow *scene_viewport_window = nullptr;
};

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
    const char* default_ini = "./assets/config/imgui_default.ini";
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

    // 加载字体
    // - 若不加载字体，dear imgui 将使用默认字体。也可加载多种字体并用 ImGui::PushFont()/PopFont() 切换。
    // - AddFontFromFileTTF() 返回 ImFont*，可在多种字体间选择时保存使用。
    // - 若文件加载失败，函数返回 nullptr，请在应用中处理（如断言或显示错误并退出）。
    // - 在 imconfig 中 #define IMGUI_ENABLE_FREETYPE 可使用 Freetype 提高字体渲染质量。
    // - 更多说明见 docs/FONTS.md。若喜欢默认字体但希望缩放更好，可考虑同作者的 'ProggyVector'。
    // - C/C++ 中字符串字面量里要包含反斜杠 \ 需写成双反斜杠 \\ 。
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    const char* font_path_chinese = nullptr;
    font_path_chinese = "./assets/ttf/HarmonyOS_Sans_SC/HarmonyOS_Sans_SC_Bold.ttf";

    if (font_path_chinese) {
        io.Fonts->AddFontFromFileTTF(font_path_chinese, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }

    *appstate = state;

    // 初始化state
    state->console_window = new ConsoleWindow();
    state->scene_tree_window = new SceneTreeWindow();
    state->gameobject_component_window = new GameobjectComponentWindow();
    state->scene_viewport_window = new SceneViewportWindow();

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
            state->scene_viewport_window->Draw();   // 场景编辑视口

        }
    }

    // 可选. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. 显示我们自定义的简单窗口，使用 Begin/End 对创建命名窗口。
    if (show_example_window)
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // 创建名为 "Hello, world!" 的窗口并向其中追加内容

        ImGui::Text("This is some useful text.");               // 显示一段文字（也可使用格式化字符串）
        ImGui::Checkbox("Demo Window", &show_demo_window);      // 用布尔值控制窗口开关状态
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // 用 0.0f～1.0f 的滑块编辑一个 float
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // 编辑表示颜色的 3 个 float

        if (ImGui::Button("Button"))                            // 按钮被点击时返回 true（多数控件在编辑/激活时返回 true）
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. 显示另一个简单窗口
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // 传入布尔变量指针，窗口有关闭按钮，点击会将该布尔置为 false
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    if (show_another_window2)
    {
        ImGui::Begin("Another Window2", &show_another_window);   // 传入布尔变量指针，窗口有关闭按钮，点击会将该布尔置为 false
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    // 渲染
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(state->gpu_device); // 获取 GPU 命令缓冲

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

    SDL_ReleaseWindowFromGPUDevice(state->gpu_device, state->window);
    SDL_DestroyGPUDevice(state->gpu_device);
    SDL_DestroyWindow(state->window);
    SDL_Quit();
}
