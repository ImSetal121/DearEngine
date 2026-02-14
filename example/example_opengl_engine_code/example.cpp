//
// Created by ImSetal on 2026/2/11.
//

#include "Engine.h"

#include <filesystem>
#include <format>
#include <memory>
#include <string>

#include "imgui.h"
#include "core/Log.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_storage.h"
#include "window/ConsoleWindow.h"
#include "window/EntityComponentWindow.h"
#include "window/SceneTreeWindow.h"
#include "window/SceneViewportWindow.h"
#include "../State.h"
#include "../application/Application.h"
#include "core/component/TestComponent.h"

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include "SDL3/SDL_opengl.h"
#endif

namespace DE {
    std::unique_ptr<Scene> editing_scene = nullptr;
    Entity* selected_entity = nullptr;
    std::unique_ptr<DA::Application> preview_application = nullptr;

    bool show_demo_window = false;
    long window_title_update_time = 0;
    static bool engine_request_quit = false;

    void CheckCurrentPath() {
        Log::Info(std::string("当前工作路径: ") + std::filesystem::current_path().string() + "\n");
    }

    std::string GetEngineAssetsPath() {
        return "./src/engine/assets/";
    }

    bool Engine::Init(void* appstate, int argc, char* argv[]) {
        auto* state = static_cast<AppState*>(appstate);

        DE::Log::Info("Welcome to Dear Engine.");
        CheckCurrentPath();

        state->console_window = new ConsoleWindow();
        state->scene_tree_window = new SceneTreeWindow();
        state->entity_component_window = new EntityComponentWindow();
        state->scene_viewport_window = new SceneViewportWindow();

        // 场景视口 FBO + 纹理（内联）
        {
            GLuint fbo = 0, tex = 0;
            glGenFramebuffers(1, &fbo);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, state->scene_viewport_texture_width, state->scene_viewport_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteTextures(1, &tex);
                glDeleteFramebuffers(1, &fbo);
                Log::Error("CreateSceneViewportFBO failed.");
                return false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            state->scene_viewport_fbo = fbo;
            state->scene_viewport_texture = tex;
        }
        state->scene_viewport_window->SetViewportTexture((void*)(intptr_t)state->scene_viewport_texture, state->scene_viewport_texture_width, state->scene_viewport_texture_height);

        // 使用 SDL_Storage 读取 default_vert.vert / default_frag.frag 并创建 program（内联）
        {
            SDL_Storage* storage = SDL_OpenFileStorage(GetEngineAssetsPath().c_str());
            if (!storage) {
                Log::Error("SDL_OpenFileStorage failed.");
                return false;
            }
            while (!SDL_StorageReady(storage))
                SDL_Delay(1);

            Uint64 vs_size = 0, fs_size = 0;
            if (!SDL_GetStorageFileSize(storage, "shader/default_vert.vert", &vs_size) || vs_size == 0 ||
                !SDL_GetStorageFileSize(storage, "shader/default_frag.frag", &fs_size) || fs_size == 0) {
                Log::Error("SDL_GetStorageFileSize failed for shader files.");
                SDL_CloseStorage(storage);
                return false;
            }

            std::string vs_src(vs_size + 1, '\0');
            std::string fs_src(fs_size + 1, '\0');
            if (!SDL_ReadStorageFile(storage, "shader/default_vert.vert", &vs_src[0], vs_size) ||
                !SDL_ReadStorageFile(storage, "shader/default_frag.frag", &fs_src[0], fs_size)) {
                Log::Error("SDL_ReadStorageFile failed for shader files.");
                SDL_CloseStorage(storage);
                return false;
            }
            vs_src[vs_size] = '\0';
            fs_src[fs_size] = '\0';
            SDL_CloseStorage(storage);

            size_t pos = 0;
            while ((pos = vs_src.find("gl_VertexIndex", pos)) != std::string::npos) {
                vs_src.replace(pos, 13, "gl_VertexID");
                pos += 11;
            }
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
                Log::Error(std::string("Vertex shader compile: ") + buf);
                glDeleteShader(vs_id);
                return false;
            }
            GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fs_id, 1, &fs_cstr, nullptr);
            glCompileShader(fs_id);
            glGetShaderiv(fs_id, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char buf[512];
                glGetShaderInfoLog(fs_id, sizeof(buf), nullptr, buf);
                Log::Error(std::string("Fragment shader compile: ") + buf);
                glDeleteShader(vs_id);
                glDeleteShader(fs_id);
                return false;
            }
            state->scene_program = glCreateProgram();
            glAttachShader(state->scene_program, vs_id);
            glAttachShader(state->scene_program, fs_id);
            glLinkProgram(state->scene_program);
            glDeleteShader(vs_id);
            glDeleteShader(fs_id);
            glGetProgramiv(state->scene_program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char buf[512];
                glGetProgramInfoLog(state->scene_program, sizeof(buf), nullptr, buf);
                Log::Error(std::string("Program link: ") + buf);
                glDeleteProgram(state->scene_program);
                state->scene_program = 0;
                return false;
            }
        }

        {
            auto test_scene = std::make_unique<Scene>();
            auto test_entity = std::make_unique<Entity>();
            test_entity->name = "entity";
            auto test_children = std::make_unique<Entity>();
            test_children->name = "children";
            auto test_entity_1 = std::make_unique<Entity>();
            test_entity_1->name = "entity_1";
            test_entity->children.push_back(std::move(test_children));
            test_entity->AddComponent<TestComponent>();
            test_scene->root.push_back(std::move(test_entity));
            test_scene->root.push_back(std::move(test_entity_1));
            editing_scene = std::move(test_scene);
        }

        return true;
    }

    bool Engine::Event(void* appstate, SDL_Event* event) {
        (void)appstate;
        (void)event;
        return true;
    }

    bool Engine::LogicIterate(void* appstate) {
        auto* state = static_cast<AppState*>(appstate);
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        Uint64 current_time_ns = SDL_GetTicksNS();
        state->delta_time_ns = current_time_ns - state->current_time_ns;
        state->current_time_ns = current_time_ns;
        state->current_time = current_time_ns / 1000000000.0;
        state->delta_time = state->delta_time_ns / 1000000000.0;

        if ((long)(state->current_time / 1.0) != window_title_update_time) {
            std::string new_title = "Dear Engine  [FPS:" + std::format("{:.2f}", io.Framerate) + "]";
            SDL_SetWindowTitle(state->engine_window, new_title.c_str());
            window_title_update_time = (long)(state->current_time / 1.0);
        }

        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking
                | ImGuiWindowFlags_MenuBar;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("MainDockHost", nullptr, host_flags);
            ImGui::PopStyleVar(3);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("文件")) {
                    if (ImGui::MenuItem("保存")) { /* ... */ }
                    ImGui::Separator();
                    if (ImGui::MenuItem("关闭")) { engine_request_quit = true; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("窗口")) {
                    ImGui::Checkbox(state->console_window->Title(), &state->console_window->open);
                    ImGui::Checkbox(state->scene_tree_window->Title(), &state->scene_tree_window->open);
                    ImGui::Checkbox(state->entity_component_window->Title(), &state->entity_component_window->open);
                    ImGui::Checkbox(state->scene_viewport_window->Title(), &state->scene_viewport_window->open);
                    ImGui::Separator();
                    ImGui::Checkbox("ImGui演示窗口", &show_demo_window);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("帮助")) {
                    if (ImGui::MenuItem("关于Dear Engine")) { /* ... */ }
                    ImGui::EndMenu();
                }
                ImGui::SameLine((ImGui::GetWindowWidth() - 80.0f) * 0.5f);
                if (ImGui::MenuItem(state->application_is_running ? "停止" : "运行")) {
                    if (state->application_is_running) {
                        state->application_is_running = false;
                    } else {
                        preview_application = std::make_unique<DA::Application>();
                        preview_application->Strat(state, editing_scene.get());
                        state->application_is_running = true;
                    }
                }
                ImGui::SameLine();
                ImGui::EndMenuBar();
            }
            ImGui::DockSpace(ImGui::GetID("MainDockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
            ImGui::End();

            state->console_window->Draw();
            state->scene_tree_window->Draw();
            state->entity_component_window->Draw();
            state->scene_viewport_window->Draw();
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);
        }

        if (GetEditingScene() == nullptr)
            DE::Log::Warning("没有正在编辑的场景.");

        if (state->application_is_running && preview_application)
            preview_application->LogicIterate(state);

        if (engine_request_quit)
            return false;
        return true;
    }

    bool Engine::RenderIterate(void* appstate) {
        auto* state = static_cast<AppState*>(appstate);

        if (state->scene_viewport_fbo && state->scene_program && state->scene_viewport_texture_width > 0 && state->scene_viewport_texture_height > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, state->scene_viewport_fbo);
            glViewport(0, 0, state->scene_viewport_texture_width, state->scene_viewport_texture_height);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            glUseProgram(state->scene_program);
            glBindVertexArray(0);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        return true;
    }

    bool Engine::Quit(void* appstate, SDL_AppResult result) {
        (void)result;
        auto* state = static_cast<AppState*>(appstate);
        selected_entity = nullptr;

        if (state->scene_viewport_texture) {
            glDeleteTextures(1, &state->scene_viewport_texture);
            state->scene_viewport_texture = 0;
        }
        if (state->scene_viewport_fbo) {
            GLuint fbo = state->scene_viewport_fbo;
            glDeleteFramebuffers(1, &fbo);
            state->scene_viewport_fbo = 0;
        }
        if (state->scene_program) {
            glDeleteProgram(state->scene_program);
            state->scene_program = 0;
        }
        return true;
    }

    Scene* Engine::GetEditingScene() {
        return editing_scene ? editing_scene.get() : nullptr;
    }

    void Engine::SetEditingScene(Scene* scene) {
        if (scene)
            editing_scene = std::make_unique<Scene>(*scene);
        else
            editing_scene = nullptr;
    }

    Entity* Engine::GetSelectedEntity() {
        return selected_entity;
    }

    void Engine::SetSelectedEntity(Entity* entity) {
        selected_entity = entity;
    }
}