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
        // 场景视口 FBO + 纹理（内联）
        {
            GLuint fbo = 0, tex = 0;
            glGenFramebuffers(1, &fbo);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scene_viewport_texture_width, scene_viewport_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
            GLuint rbo = 0;
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                scene_viewport_texture_width, scene_viewport_texture_height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            scene_viewport_rbo = rbo;
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteRenderbuffers(1, &rbo);
                glDeleteTextures(1, &tex);
                glDeleteFramebuffers(1, &fbo);
                DE::Log::Error("CreateSceneViewportFBO failed.");
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            scene_viewport_fbo = fbo;
            scene_viewport_texture = tex;
        }
        SetViewportTexture((void*)(intptr_t)scene_viewport_texture, scene_viewport_texture_width, scene_viewport_texture_height);

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
        camera = new ICamera();
        render_context->camera = camera;
        camera->Position = glm::vec3(0.0f, 0.0f, 5.0f);   // 放在场景前方
        printf("Viewport camera position: %.1f %.1f %.1f\n", camera->Position.x, camera->Position.y, camera->Position.z);
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
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_engine_window = this;

        if (viewport_texture_ && viewport_width_ && viewport_height_) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.x > 1) viewport_width_ = avail.x;
            if (avail.y > 1) viewport_height_ = avail.y;
            ImGui::Image(viewport_texture_, ImVec2(viewport_width_, viewport_height_));
        }
        ImGui::End();
        return true;
    }

    bool SceneViewportWindow::RenderIterate(void *appstate) {
        if (scene_viewport_fbo && scene_program && scene_viewport_texture_width > 0 && scene_viewport_texture_height > 0) {
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
}