//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_PATH_H
#define DEARENGINE_PATH_H
#include <filesystem>
#include <string>

#include "../core/Log.h"

namespace DE {
    inline void CheckCurrentPath() {
        Log::Info(std::string("当前工作路径: ") + std::filesystem::current_path().string()+"\n");
    }

    inline std::string GetEngineAssetsPath() {
        return "./src/engine/assets/";
    }
}

#endif //DEARENGINE_PATH_H