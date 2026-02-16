//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_PATH_H
#define DEARENGINE_PATH_H
#include <filesystem>
#include <string>

#include "../core/Log.h"

namespace DE {
    /** 项目内容根目录（编译期由 CMake 注入，不依赖 cwd） */
    inline std::filesystem::path GetContentRoot() {
#define DE_STRINGIFY(x) #x
#define DE_TOSTRING(x)  DE_STRINGIFY(x)
        std::string raw(DE_TOSTRING(CONTENT_ROOT));
#undef DE_TOSTRING
#undef DE_STRINGIFY
        // 若宏里误带了引号，去掉首尾双引号
        if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
            raw = raw.substr(1, raw.size() - 2);
        return std::filesystem::path(raw);
    }

    /** 相对内容根的路径，例如 AssetPath("shader/xxx.vert") */
    inline std::filesystem::path AssetPath(const std::string& rel) {
        return GetContentRoot() / rel;
    }

    /** 引擎资源目录（src/engine/assets/），返回字符串便于接老接口（如 SDL_Storage） */
    inline std::string GetEngineAssetsPath() {
        return (GetContentRoot() / "src" / "engine" / "assets").generic_string() + "/";
    }

    inline void CheckCurrentPath() {
        Log::Info(std::string("当前内容根路径: ") + GetContentRoot().string()+"\n");
    }
}

#endif //DEARENGINE_PATH_H