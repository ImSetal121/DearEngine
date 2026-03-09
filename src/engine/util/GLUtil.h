//
// Created by ImSetal on 2026/3/4.
//

#ifndef DEARENGINE_GLUTIL_H
#define DEARENGINE_GLUTIL_H
#include "Path.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "glad/glad.h"

namespace DE {
    inline const char *SelectGLVersion() {
        // 选择 GL 与 GLSL 版本
#if defined(__APPLE__)
        const char* glsl_version = "#version 410";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Mac 上必须设置
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
        const char* glsl_version = "#version 330";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

        return glsl_version;
    }

    inline void CreateProgram(unsigned int &program, char* vert_path, char* frag_path) {

        // 使用 SDL_Storage 读取 default_scene_vert.vert / default_scene_frag.frag 并创建 program（内联）
        SDL_Storage* storage = SDL_OpenFileStorage(DE::GetEngineAssetsPath().c_str());
        if (!storage) {
            DE::Log::Error("SDL_OpenFileStorage failed.");
        }
        while (!SDL_StorageReady(storage))
            SDL_Delay(1);

        Uint64 vs_size = 0, fs_size = 0;
        if (!SDL_GetStorageFileSize(storage, vert_path, &vs_size) || vs_size == 0 ||
            !SDL_GetStorageFileSize(storage, frag_path, &fs_size) || fs_size == 0) {
            DE::Log::Error("SDL_GetStorageFileSize failed for shader files.");
            SDL_CloseStorage(storage);
            }

        std::string vs_src(vs_size + 1, '\0');
        std::string fs_src(fs_size + 1, '\0');
        if (!SDL_ReadStorageFile(storage, vert_path, &vs_src[0], vs_size) ||
            !SDL_ReadStorageFile(storage, frag_path, &fs_src[0], fs_size)) {
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
        program = glCreateProgram();
        glAttachShader(program, vs_id);
        glAttachShader(program, fs_id);
        glLinkProgram(program);
        glDeleteShader(vs_id);
        glDeleteShader(fs_id);
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetProgramInfoLog(program, sizeof(buf), nullptr, buf);
            DE::Log::Error(std::string("Program link: ") + buf);
            glDeleteProgram(program);
            program = 0;
        }
    }
}

#endif //DEARENGINE_GLUTIL_H