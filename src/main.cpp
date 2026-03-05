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
#include "engine/editor/EngineEditor.h"
#include "engine/util/Path.h"
#include "glad/glad.h"
#include <glm/glm.hpp>

#include "engine/util/GLUtil.h"

#ifdef _WIN32
#include <windows.h>
#endif

bool hasArgv(int argc, char *argv[], std::string value) {
    for (int i = 0; i < argc; ++i) {
        if (value == argv[i])
            return true;
    }
    return false;
}

void CreateDefaultProgram(void *appstate) {
    auto state = static_cast<AppState *>(appstate);

    // 使用 SDL_Storage 读取 default_scene_vert.vert / default_scene_frag.frag 并创建 program（内联）
    SDL_Storage* storage = SDL_OpenFileStorage(DE::GetEngineAssetsPath().c_str());
    if (!storage) {
        DE::Log::Error("SDL_OpenFileStorage failed.");
    }
    while (!SDL_StorageReady(storage))
        SDL_Delay(1);

    Uint64 vs_size = 0, fs_size = 0;
    if (!SDL_GetStorageFileSize(storage, "shader/default_scene_vert.vert", &vs_size) || vs_size == 0 ||
        !SDL_GetStorageFileSize(storage, "shader/default_scene_frag.frag", &fs_size) || fs_size == 0) {
        DE::Log::Error("SDL_GetStorageFileSize failed for shader files.");
        SDL_CloseStorage(storage);
        }

    std::string vs_src(vs_size + 1, '\0');
    std::string fs_src(fs_size + 1, '\0');
    if (!SDL_ReadStorageFile(storage, "shader/default_scene_vert.vert", &vs_src[0], vs_size) ||
        !SDL_ReadStorageFile(storage, "shader/default_scene_frag.frag", &fs_src[0], fs_size)) {
        DE::Log::Error("SDL_ReadStorageFile failed for shader files.");
        SDL_CloseStorage(storage);
        }
    vs_src[vs_size] = '\0';
    fs_src[fs_size] = '\0';
    SDL_CloseStorage(storage);

    // size_t pos = 0;
    // while ((pos = vs_src.find("gl_VertexIndex", pos)) != std::string::npos) {
    //     vs_src.replace(pos, 14, "gl_VertexID");
    //     pos += 11;
    // }
    const char* vs_cstr = vs_src.c_str();
    const char* fs_cstr = fs_src.c_str();

    GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_id, 1, &vs_cstr, nullptr);
    glCompileShader(vs_id);
    GLint ok = 0;
    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(vs_id, sizeof(buf), nullptr, buf);
        DE::Log::Error(std::string("Vertex shader compile: ") + buf);
        glDeleteShader(vs_id);
    }
    GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_id, 1, &fs_cstr, nullptr);
    glCompileShader(fs_id);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(fs_id, sizeof(buf), nullptr, buf);
        DE::Log::Error(std::string("Fragment shader compile: ") + buf);
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
    }
    state->default_program = glCreateProgram();
    glAttachShader(state->default_program, vs_id);
    glAttachShader(state->default_program, fs_id);
    glLinkProgram(state->default_program);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    glGetProgramiv(state->default_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(state->default_program, sizeof(buf), nullptr, buf);
        DE::Log::Error(std::string("Program link: ") + buf);
        glDeleteProgram(state->default_program);
        state->default_program = 0;
    }
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);  // 控制台输出使用 UTF-8
    SetConsoleCP(65001);       // 控制台输入也用 UTF-8（若需要从控制台读入）
#endif

    // 初始化AppState
    auto *state = new AppState();

    if (hasArgv(argc, argv, "-editor")) {
        state->edit_mode = true;
        printf("Editor mode enabled.\n");
    }

    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const char *glsl_version = DE::SelectGLVersion();

    // 创建 SDL 窗口与图形上下文
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    state->editor_window = SDL_CreateWindow("Dear Engine Editor", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (state->editor_window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(state->editor_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    state->application_window = SDL_CreateWindow("Dear Application", (int)(854 * main_scale), (int)(480 * main_scale), window_flags);
    if (state->application_window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(state->application_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    state->gl_context = SDL_GL_CreateContext(state->application_window); // 创建gl上下文
    if (state->gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GL_SetSwapInterval(1); // 开启垂直同步

    // 加载GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Error: gladLoadGLLoader failed.\n");
        return SDL_APP_FAILURE;
    }

    // 创建默认着色程序
    CreateDefaultProgram(state);

    if (state->edit_mode) {
        SDL_ShowWindow(state->editor_window);

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
        // ImGui::StyleColorsLight();

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
        ImGui_ImplSDL3_InitForOpenGL(state->editor_window, state->gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // 加载字体
        std::string font_path_str = DE::GetEngineAssetsPath() + "ttf/HarmonyOS_Sans_SC/HarmonyOS_Sans_SC_Regular.ttf";
        const char* font_path_chinese = font_path_str.c_str();
        if (font_path_chinese)
            io.Fonts->AddFontFromFileTTF(font_path_chinese, 16.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

        // 引擎Init
        if (!DE::EngineEditor::Init(state, argc, argv)) {
            return SDL_APP_FAILURE;
        }

    }else {

        state->application = std::make_unique<DA::Application>();
        if (!state->application->Start(state, nullptr, argc, argv)) {
            return SDL_APP_FAILURE;
        }
        state->application_is_running = true;

    }

    // 返回AppState指针
    *appstate = state;

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    auto *state = static_cast<AppState *>(appstate);

    if (state->application_is_running && state->application) {
        if (!state->application->Event(appstate, event)) {
            state->application->End(appstate);
            if (!state->edit_mode)
                return SDL_APP_SUCCESS;
        }
    }

    if (state->edit_mode) {
        // 轮询并处理事件（输入、窗口缩放等）
        // 可通过 io.WantCaptureMouse、io.WantCaptureKeyboard 判断 dear imgui 是否要接管输入。
        // - io.WantCaptureMouse 为 true 时，不要将鼠标输入发给主应用，或清空/覆盖你的鼠标数据副本。
        // - io.WantCaptureKeyboard 为 true 时，不要将键盘输入发给主应用，或清空/覆盖你的键盘数据副本。
        // 通常可始终把所有输入传给 dear imgui，再根据这两个标志决定是否对主应用隐藏。
        ImGui_ImplSDL3_ProcessEvent(event);
        if (event->type == SDL_EVENT_QUIT)
            return SDL_APP_SUCCESS;
        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event->window.windowID == SDL_GetWindowID(state->editor_window))
            return SDL_APP_SUCCESS;

        if (!DE::EngineEditor::Event(appstate, event))
            return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;

}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    if (SDL_GetWindowFlags(state->editor_window) & SDL_WINDOW_MINIMIZED)
    {
        SDL_Delay(10);
        return SDL_APP_CONTINUE;;
    }

    // 应用程序逻辑迭代
    if (state->application_is_running == false && state->application) { //检查是否未关闭
        state->application->End(state);
        state->application.reset();
    }
    if (state->application_is_running && state->application) {
        state->application->LogicIterate(state);
    }

    // 引擎编辑器逻辑迭代
    if (state->edit_mode) {
        // 开始 Dear ImGui 本帧
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        if (!DE::EngineEditor::LogicIterate(appstate))
            return SDL_APP_FAILURE;
    }


    // 应用程序渲染迭代
    if (state->application_is_running && state->application) {
        state->application->RenderIterate(state);
    }

    // 引擎编辑器渲染迭代
    if (state->edit_mode) {
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::Render();
        // 将主窗口的 framebuffer 设为当前，并清屏
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!DE::EngineEditor::RenderIterate(appstate))
            return SDL_APP_FAILURE;
        // 把 ImGui 画到当前默认 framebuffer
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

        SDL_GL_SwapWindow(state->editor_window);
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    auto *state = static_cast<AppState *>(appstate);
    if (state->edit_mode) {
        // 清理
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        DE::EngineEditor::Quit(appstate, result);
        SDL_DestroyWindow(state->editor_window);
    }

    SDL_DestroyWindow(state->application_window);
    SDL_GL_DestroyContext(state->gl_context);
    SDL_Quit();
}
