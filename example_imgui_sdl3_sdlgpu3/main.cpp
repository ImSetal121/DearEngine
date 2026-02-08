// Dear ImGui：SDL3 + SDL_GPU 独立示例程序
// （SDL 是跨平台通用库，用于窗口、输入、OpenGL/Vulkan/Metal 图形上下文创建等。）

// 了解 Dear ImGui：
// - 常见问题          https://dearimgui.com/faq
// - 入门指南          https://dearimgui.com/getting-started
// - 文档              https://dearimgui.com/docs（与本地 docs/ 目录相同）
// - 简介与更多链接见 imgui.cpp 顶部

// 若要将 imgui_impl_sdlgpu3.cpp/.h 集成到自己的引擎/应用中，请注意：
// - 与其他后端不同，用户必须在发起包含 ImGui_ImplSDLGPU_RenderDrawData 的 SDL_GPURenderPass 之前调用 ImGui_ImplSDLGPU_PrepareDrawData()。
//   该调用是必须的，否则 ImGui 不会向 GPU 上传顶点缓冲或索引缓冲。详见 imgui_impl_sdlgpu3.cpp。

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort 等
#include <SDL3/SDL.h>

// 本示例尚无法用 Emscripten 编译，等待 SDL3 支持。
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// 主程序
int main(int, char**)
{
    // 初始化 SDL
    // [若使用 SDL_MAIN_USE_CALLBACKS：下面直到主循环开始前的代码可放在 SDL_AppInit() 中]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // 创建 SDL 窗口与图形上下文
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_GPU example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // 创建 GPU 设备
    SDL_GPUDevice* gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB,true,nullptr);
    if (gpu_device == nullptr)
    {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }

    // 将窗口交由 GPU 设备使用
    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window))
    {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetGPUSwapchainParameters(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    // 设置 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
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
    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
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

    // 本示例状态
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 主循环
    bool done = false;
    while (!done)
    {
        // 轮询并处理事件（输入、窗口缩放等）
        // 可通过 io.WantCaptureMouse、io.WantCaptureKeyboard 判断 dear imgui 是否要接管输入。
        // - io.WantCaptureMouse 为 true 时，不要将鼠标输入发给主应用，或清空/覆盖你的鼠标数据副本。
        // - io.WantCaptureKeyboard 为 true 时，不要将键盘输入发给主应用，或清空/覆盖你的键盘数据副本。
        // 通常可始终把所有输入传给 dear imgui，再根据这两个标志决定是否对主应用隐藏。
        // [若使用 SDL_MAIN_USE_CALLBACKS：在 SDL_AppEvent() 中调用 ImGui_ImplSDL3_ProcessEvent()]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [若使用 SDL_MAIN_USE_CALLBACKS：下面代码可放在 SDL_AppIterate() 中]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // 开始 Dear ImGui 本帧
        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. 显示大型演示窗口（大部分示例代码在 ImGui::ShowDemoWindow() 中，可浏览其代码以进一步了解 Dear ImGui）。
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. 显示我们自定义的简单窗口，使用 Begin/End 对创建命名窗口。
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

        // 渲染
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device); // 获取 GPU 命令缓冲

        SDL_GPUTexture* swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr); // 获取交换链纹理

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
    }

    // 清理
    // [若使用 SDL_MAIN_USE_CALLBACKS：下面代码可放在 SDL_AppQuit() 中]
    SDL_WaitForGPUIdle(gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
