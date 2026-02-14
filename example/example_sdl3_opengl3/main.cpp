// Dear ImGui：SDL3 + OpenGL 独立示例程序
// （SDL 是跨平台通用库，用于处理窗口、输入、OpenGL/Vulkan/Metal 图形上下文创建等。）

// 了解 Dear ImGui：
// - 常见问题           https://dearimgui.com/faq
// - 入门指南           https://dearimgui.com/getting-started
// - 文档               https://dearimgui.com/docs（与本地 docs/ 目录相同）。
// - 简介、链接等见 imgui.cpp 文件顶部

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// 主程序
int main(int, char**)
{
    // 初始化 SDL
    // [若使用 SDL_MAIN_USE_CALLBACKS：主循环开始前的以下代码可放入 SDL_AppInit() 中]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // 选择 GL 与 GLSL 版本
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100（WebGL 1.0）
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es（WebGL 2.0）
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Mac 上必须设置
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130（桌面 OpenGL）
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // 创建带图形上下文的窗口
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // 开启垂直同步
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // 初始化 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 启用键盘控制
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // 启用手柄控制
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // 启用停靠
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // 启用多视口 / 平台窗口
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // 设置 Dear ImGui 样式
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // 设置缩放
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // 固定样式缩放（在支持动态样式缩放前，修改需先重置 Style 再调用此函数）
    style.FontScaleDpi = main_scale;        // 设置初始字体缩放（io.ConfigDpiScaleFonts=true 时可省略，此处保留作说明）
    io.ConfigDpiScaleFonts = true;          // [实验] 显示器 DPI 变化时在 Begin() 中自动覆盖 style.FontScaleDpi，目前只缩放字体，不缩放尺寸/内边距。
    io.ConfigDpiScaleViewports = true;     // [实验] 显示器 DPI 变化时缩放 Dear ImGui 与平台窗口。

    // 启用视口时调整 WindowRounding/WindowBg，使平台窗口与普通窗口外观一致。
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 设置平台/渲染器后端
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 加载字体
    // - 未加载字体时，dear imgui 使用默认字体。也可加载多种字体并用 ImGui::PushFont()/PopFont() 切换。
    // - AddFontFromFileTTF() 返回 ImFont*，可在多种字体间选择时保存使用。
    // - 若文件加载失败，函数返回 nullptr，请在程序中处理（如断言或显示错误并退出）。
    // - 在 imconfig 中定义 IMGUI_ENABLE_FREETYPE 可使用 Freetype 获得更高质量字体渲染。
    // - 更多说明见 docs/FONTS.md。若喜欢默认字体但希望缩放更好，可考虑同作者的 'ProggyVector'。
    // - C/C++ 中字符串字面量里要写反斜杠 \ 需写成双反斜杠 \\。
    // - Emscripten 构建可将字体嵌入，运行时从 "fonts/" 目录访问，详见 Makefile.emscripten。
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // 程序状态
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 主循环
    bool done = false;
#ifdef __EMSCRIPTEN__
    // Emscripten 构建中禁用文件系统访问，因此不尝试对 imgui.ini 执行 fopen()。
    // 可手动调用 LoadIniSettingsFromMemory() 从自有存储加载配置。
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // 轮询并处理事件（输入、窗口缩放等）
        // 可通过 io.WantCaptureMouse、io.WantCaptureKeyboard 判断 dear imgui 是否要接管输入。
        // - io.WantCaptureMouse 为 true 时，不要将鼠标输入发给主程序，或清除/覆盖你的鼠标数据副本。
        // - io.WantCaptureKeyboard 为 true 时，不要将键盘输入发给主程序，或清除/覆盖你的键盘数据副本。
        // 通常可始终把所有输入传给 dear imgui，再根据这两个标志决定是否对主程序隐藏。
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

        // [若使用 SDL_MAIN_USE_CALLBACKS：以下代码可放入 SDL_AppIterate() 中]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // 开始 Dear ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 1. 显示主演示窗口（大部分示例在 ImGui::ShowDemoWindow() 中，可浏览其代码学习 Dear ImGui）。
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. 显示我们自定义的简单窗口，用 Begin/End 对创建命名窗口。
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // 创建名为 "Hello, world!" 的窗口并往其中追加内容。

            ImGui::Text("This is some useful text.");               // 显示文本（也可使用格式字符串）
            ImGui::Checkbox("Demo Window", &show_demo_window);      // 编辑控制窗口开关的 bool
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

        // 3. 显示另一个简单窗口。
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // 传入 bool 变量指针，窗口有关闭按钮，点击会清除该 bool
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // 渲染
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 更新并渲染额外的平台窗口
        // （平台函数可能改变当前 OpenGL 上下文，故保存/恢复以便代码可复用到别处；
        //  本示例也可直接调用 SDL_GL_MakeCurrent(window, gl_context)。）
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // 清理
    // [若使用 SDL_MAIN_USE_CALLBACKS：以下代码可放入 SDL_AppQuit() 中]
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
