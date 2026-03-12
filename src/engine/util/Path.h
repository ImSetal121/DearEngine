//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_PATH_H
#define DEARENGINE_PATH_H
#include <filesystem>
#include <string>
#include <SDL3/SDL_filesystem.h>

#include "../core/Log.h"

namespace DE {
    /** 是否从“安装目录”运行（使用 exe 旁的 Content/ 或 DE_CONTENT_ROOT），
     * 否则为开发布局（使用编译期 CONTENT_ROOT）。用于在代码中区分安装/开发。 */
    inline bool IsRunningFromInstallDir();

    /** 运行时内容根（首次调用时解析并缓存）。
     * 顺序：环境变量 DE_CONTENT_ROOT → exe 旁 Content/ → 编译期 CONTENT_ROOT。 */
    inline const std::filesystem::path &GetContentRoot() {
        static std::filesystem::path s_cached;
        static bool s_install_layout = false; // 是否安装布局
        static bool s_resolved = false;
        if (s_resolved) {
            return s_cached;
        }
        const char *env_root = std::getenv("DE_CONTENT_ROOT");
        if (env_root && env_root[0] != '\0') {
            s_cached = std::filesystem::path(env_root).lexically_normal();
            s_install_layout = !std::filesystem::exists(s_cached / "src" / "engine" / "assets");
            s_resolved = true;
            return s_cached;
        }
        const char *base = SDL_GetBasePath();
        if (base) {
            std::filesystem::path content = (std::filesystem::path(base) / "Content").lexically_normal();
            if (std::filesystem::exists(content)) {
                s_cached = content;
                s_install_layout = true;
                s_resolved = true;
                return s_cached;
            }
        }
#define DE_STRINGIFY(x) #x
#define DE_TOSTRING(x)  DE_STRINGIFY(x)
        std::string raw(DE_TOSTRING(CONTENT_ROOT));
#undef DE_TOSTRING
#undef DE_STRINGIFY
        if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"')
            raw = raw.substr(1, raw.size() - 2);
        s_cached = std::filesystem::path(raw).lexically_normal();
        s_install_layout = false;
        s_resolved = true;
        return s_cached;
    }

    inline bool IsRunningFromInstallDir() {
        (void) GetContentRoot(); // 确保已解析
        static bool s_install_layout = false;
        static bool s_done = false;
        if (!s_done) {
            const auto &root = GetContentRoot();
            s_install_layout = !std::filesystem::exists(root / "src" / "engine" / "assets");
            s_done = true;
        }
        return s_install_layout;
    }

    inline std::filesystem::path AssetPath(const std::string &rel) {
        return GetContentRoot() / rel;
    }

    inline std::string GetEngineAssetsPath() {
        const auto &root = GetContentRoot();
        if (std::filesystem::exists(root / "src" / "engine" / "assets"))
            return (root / "src" / "engine" / "assets").generic_string() + "/";
        return (root / "engine" / "assets").generic_string() + "/";
    }

    inline std::string GetApplicationAssetsPath() {
        const auto &root = GetContentRoot();
        if (std::filesystem::exists(root / "src" / "application" / "assets"))
            return (root / "src" / "application" / "assets").generic_string() + "/";
        return (root / "application" / "assets").generic_string() + "/";
    }

    inline void CheckCurrentPath() {
        Log::Info(std::string("当前内容根路径: ") + GetContentRoot().string() + "\n");
    }
}

#endif //DEARENGINE_PATH_H
