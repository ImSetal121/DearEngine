#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "State.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include "engine/Engine.h"
#include "engine/util/Path.h"
#include "glad/glad.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);  // 控制台输出使用 UTF-8
    SetConsoleCP(65001);       // 控制台输入也用 UTF-8（若需要从控制台读入）
#endif

    std::printf("指令个数: %i\n", argc);
    for (int i = 0; i < argc; ++i) {
        std::printf("  [%i] %s\n", i, argv[i]);
    }

    std::printf("本设备支持的图形驱动:");
    std::vector<char*> devices;
    for (int i = 0; i < SDL_GetNumGPUDrivers(); i++) {
        std::printf(" %s", SDL_GetGPUDriver(i));
        if (i == SDL_GetNumGPUDrivers() - 1) std::printf(".\n");
        else std::printf(",");
    }

    // 初始化AppState
    auto *state = new AppState();

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
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
    const char* glsl_version = "#version 410";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Mac 上必须设置
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    // GL 3.0 + GLSL 130（桌面 OpenGL）
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // 创建 SDL 窗口与图形上下文
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    state->engine_window = SDL_CreateWindow("Dear Engine", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (state->engine_window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    state->gl_context = SDL_GL_CreateContext(state->engine_window);
    if (state->gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GL_MakeCurrent(state->engine_window, state->gl_context);
    SDL_GL_SetSwapInterval(1); // 开启垂直同步
    SDL_SetWindowPosition(state->engine_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(state->engine_window);

    // 加载GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Error: gladLoadGLLoader failed.\n");
        return SDL_APP_FAILURE;
    }

    // 设置 Dear ImGui 上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // ImGui 布局：若用户尚未有 imgui.ini，则从默认模板复制
    const char* user_ini = "imgui.ini";
    std::string ini_path = DE::GetEngineAssetsPath() + "config/imgui_default.ini";
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
    ImGui_ImplSDL3_InitForOpenGL(state->engine_window, state->gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 加载字体
    std::string font_path_str = DE::GetEngineAssetsPath() + "ttf/HarmonyOS_Sans_SC/HarmonyOS_Sans_SC_Regular.ttf";
    const char* font_path_chinese = font_path_str.c_str();
    if (font_path_chinese)
        io.Fonts->AddFontFromFileTTF(font_path_chinese, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    // 引擎Init
    if (!DE::Engine::Init(state, argc, argv)) {
        return SDL_APP_FAILURE;
    }

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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 引擎逻辑tick
    if (!DE::Engine::LogicIterate(appstate))
        return SDL_APP_FAILURE;

    // 渲染
    ImGui::Render();
    // 1) 将主窗口的 framebuffer 设为当前，并清屏
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    // 2) 引擎渲染
    if (!DE::Engine::RenderIterate(appstate))
        return SDL_APP_FAILURE;
    // 3) 把 ImGui 画到当前默认 framebuffer
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

    SDL_GL_SwapWindow(state->engine_window);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *state = static_cast<AppState *>(appstate);
    // 清理
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    DE::Engine::Quit(appstate, result);

    SDL_GL_DestroyContext(state->gl_context);
    SDL_DestroyWindow(state->engine_window);
    SDL_Quit();
}
