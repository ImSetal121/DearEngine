//
// Created by ImSetal on 2026/2/8.
//

#include "SceneViewportWindow.h"
#include "imgui.h"
#include "../Engine.h"
#include "../../State.h"
#include "../core/Log.h"
#include "../core/Scene.h"
#include "../util/Path.h"
#include "glad/glad.h"
#include "SDL3/SDL_storage.h"
#include "SDL3/SDL_timer.h"

namespace DE {
    SceneViewportWindow::SceneViewportWindow() = default;

    // 递归：对单个实体及其所有子实体的组件调用 RenderIterate
    static void RenderIterateEntity(DE::Entity* entity, void* appstate, RenderContext *render_context) {
        if (!entity) return;
        for (auto& kv : entity->components)
            kv.second->RenderIterate(appstate, render_context);
        for (auto& child : entity->children)
            RenderIterateEntity(child.get(), appstate, render_context);
    }

    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* SceneViewportWindow::Title() const {
        return "场景视口";
    }

    bool SceneViewportWindow::Init(void *appstate) {
        auto state = static_cast<AppState*>(appstate);
        // 场景视口 FBO + 附件（封装到 EnsureFboAttachments）
        {
            GLuint fbo = 0;
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            if (!EnsureFboAttachments(scene_viewport_texture_width, scene_viewport_texture_height)) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &fbo);
                DE::Log::Error("CreateSceneViewportFBO failed.");
                return false;
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            scene_viewport_fbo = fbo;
        }

        // 使用 SDL_Storage 读取 default_scene_vert.vert / default_scene_frag.frag 并创建 program（内联）
        {
            SDL_Storage* storage = SDL_OpenFileStorage(DE::GetEngineAssetsPath().c_str());
            if (!storage) {
                DE::Log::Error("SDL_OpenFileStorage failed.");
                return false;
            }
            while (!SDL_StorageReady(storage))
                SDL_Delay(1);

            Uint64 vs_size = 0, fs_size = 0;
            if (!SDL_GetStorageFileSize(storage, "shader/default_scene_vert.vert", &vs_size) || vs_size == 0 ||
                !SDL_GetStorageFileSize(storage, "shader/default_scene_frag.frag", &fs_size) || fs_size == 0) {
                DE::Log::Error("SDL_GetStorageFileSize failed for shader files.");
                SDL_CloseStorage(storage);
                return false;
                }

            std::string vs_src(vs_size + 1, '\0');
            std::string fs_src(fs_size + 1, '\0');
            if (!SDL_ReadStorageFile(storage, "shader/default_scene_vert.vert", &vs_src[0], vs_size) ||
                !SDL_ReadStorageFile(storage, "shader/default_scene_frag.frag", &fs_src[0], fs_size)) {
                DE::Log::Error("SDL_ReadStorageFile failed for shader files.");
                SDL_CloseStorage(storage);
                return false;
                }
            vs_src[vs_size] = '\0';
            fs_src[fs_size] = '\0';
            SDL_CloseStorage(storage);

            size_t pos = 0;
            while ((pos = vs_src.find("gl_VertexIndex", pos)) != std::string::npos) {
                vs_src.replace(pos, 14, "gl_VertexID");
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
                DE::Log::Error(std::string("Vertex shader compile: ") + buf);
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
                DE::Log::Error(std::string("Fragment shader compile: ") + buf);
                glDeleteShader(vs_id);
                glDeleteShader(fs_id);
                return false;
            }
            scene_program = glCreateProgram();
            glAttachShader(scene_program, vs_id);
            glAttachShader(scene_program, fs_id);
            glLinkProgram(scene_program);
            glDeleteShader(vs_id);
            glDeleteShader(fs_id);
            glGetProgramiv(scene_program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char buf[512];
                glGetProgramInfoLog(scene_program, sizeof(buf), nullptr, buf);
                DE::Log::Error(std::string("Program link: ") + buf);
                glDeleteProgram(scene_program);
                scene_program = 0;
                return false;
            }
        }
        state->scene_program = &scene_program;
        glGenVertexArrays(1, &scene_vao);

        render_context = new RenderContext();
        camera = new ICamera(&camera_position, &camera_rotation);
        render_context->camera = camera;
        render_context->program = &scene_program;
        render_context->screenWidth = &scene_viewport_texture_width;
        render_context->screenHeight = &scene_viewport_texture_height;

        return IEngineWindow::Init(appstate);
    }

    bool SceneViewportWindow::Event() {
        return IEngineWindow::Event();
    }

    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    bool SceneViewportWindow::LogicIterate(void *appstate) {
        if (!open) return false;
        auto state = static_cast<AppState*>(appstate);
        if (state->application_is_running)
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

        ImGui::Begin(Title(), nullptr, ImGuiWindowFlags_MenuBar);
        // 检查是否为焦点窗口
        if (ImGui::IsWindowFocused())
            state->focused_engine_window = this;

        // 最上方固定菜单栏
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("视图"))
            {
                if (ImGui::MenuItem("透视")) { /* ... */ }
                if (ImGui::MenuItem("正交")) { /* ... */ }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("渲染"))
            {
                ImGui::Separator();
                // ImGui::Checkbox("无光照模型", nullptr);
                // ImGui::Checkbox("Blinn-Phong模型", nullptr);
                // ImGui::Checkbox("PBR模型", nullptr);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Camera"))
            {
                ImGui::SetNextItemWidth(60);
                ImGui::DragFloat("Sensitivity", &camera_sensitivity, 0.01, 0, 3, "%.2f");
                ImGui::SetNextItemWidth(60);
                ImGui::DragFloat("Move Speed", &camera_move_speed, 0.1, 0, 100, "%.2f");
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // 场景视口相机控制
        bool is_hovered = ImGui::IsWindowHovered();

        if (camera && is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            ImGuiIO& io = ImGui::GetIO();
            // 鼠标旋转
            if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
                camera_rotation.y += io.MouseDelta.x * camera_sensitivity;  // yaw
                camera_rotation.x -= io.MouseDelta.y * camera_sensitivity;  // pitch
                camera_rotation.x = glm::clamp(camera_rotation.x, -89.0f, 89.0f);
            }

            // WASD 移动
            float speed = camera_move_speed * state->delta_time;
            if (ImGui::IsKeyDown(ImGuiKey_W)) camera_position += camera->GetFront() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) camera_position -= camera->GetFront() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) camera_position -= camera->GetRight() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) camera_position += camera->GetRight() * speed;
            if (ImGui::IsKeyDown(ImGuiKey_E)) camera_position.y += speed;
            if (ImGui::IsKeyDown(ImGuiKey_Q)) camera_position.y -= speed;
        }

        if (viewport_texture_) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            int w = (avail.x > 1) ? (int)avail.x : viewport_width_;
            int h = (avail.y > 1) ? (int)avail.y : viewport_height_;
            if (w < 1) w = 1;
            if (h < 1) h = 1;
            viewport_width_ = w;
            viewport_height_ = h;
            ImGui::Image(viewport_texture_,
                         ImVec2((float)viewport_width_, (float)viewport_height_),
                         ImVec2(0, 1),  // uv0: 左上角取纹理的 (0, 1)
                         ImVec2(1, 0)); // uv1: 右下角取纹理的 (1, 0)
        }
        ImGui::End();
        if (state->application_is_running)
            ImGui::PopStyleColor();
        return true;
    }

    bool SceneViewportWindow::RenderIterate(void *appstate) {
        if (!scene_viewport_fbo || !scene_program) return true;

        int target_w = viewport_width_ > 0 ? viewport_width_ : scene_viewport_texture_width;
        int target_h = viewport_height_ > 0 ? viewport_height_ : scene_viewport_texture_height;
        if (target_w < 1) target_w = 1;
        if (target_h < 1) target_h = 1;

        if (target_w != scene_viewport_texture_width || target_h != scene_viewport_texture_height) {
            glBindFramebuffer(GL_FRAMEBUFFER, scene_viewport_fbo);
            if (!EnsureFboAttachments(target_w, target_h)) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return true;
            }
        }

        if (scene_viewport_texture_width > 0 && scene_viewport_texture_height > 0) {
            glBindFramebuffer(GL_FRAMEBUFFER, scene_viewport_fbo);
            glViewport(0, 0, scene_viewport_texture_width, scene_viewport_texture_height);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(scene_program);
            glEnable(GL_DEPTH_TEST);

            Scene* current_editing_scene = Engine::GetEditingScene();
            if (current_editing_scene) {
                for (auto& entity : current_editing_scene->root) {
                    RenderIterateEntity(entity.get(), appstate, render_context);
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        return true;
    }

    bool SceneViewportWindow::Quit() {
        if (scene_viewport_texture) {
            glDeleteTextures(1, &scene_viewport_texture);
            scene_viewport_texture = 0;
        }
        if (scene_viewport_rbo) {
            GLuint rbo = scene_viewport_rbo;
            glDeleteRenderbuffers(1, &rbo);
            scene_viewport_rbo = 0;
        }
        if (scene_viewport_fbo) {
            GLuint fbo = scene_viewport_fbo;
            glDeleteFramebuffers(1, &fbo);
            scene_viewport_fbo = 0;
        }
        if (scene_program) {
            glDeleteProgram(scene_program);
            scene_program = 0;
        }
        if (scene_vao) {
            glDeleteVertexArrays(1, &scene_vao);
            scene_vao = 0;
        }
        return IEngineWindow::Quit();
    }

    /** 设置视口绘制纹理指针 */
    void SceneViewportWindow::SetViewportTexture(void *texture, int width, int height) {
        viewport_texture_ = texture;
        viewport_width_ = width;
        viewport_height_ = height;
    }

    bool SceneViewportWindow::EnsureFboAttachments(int width, int height) {
        if (width < 1 || height < 1) return false;

        if (scene_viewport_texture) {
            glDeleteTextures(1, &scene_viewport_texture);
            scene_viewport_texture = 0;
        }
        if (scene_viewport_rbo) {
            glDeleteRenderbuffers(1, &scene_viewport_rbo);
            scene_viewport_rbo = 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        GLuint rbo = 0;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        scene_viewport_texture = tex;
        scene_viewport_rbo = rbo;
        scene_viewport_texture_width = width;
        scene_viewport_texture_height = height;
        SetViewportTexture((void*)(intptr_t)scene_viewport_texture, scene_viewport_texture_width, scene_viewport_texture_height);

        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
}