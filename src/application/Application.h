//
// Created by ImSetal on 2026/2/12.
//

#ifndef DEARENGINE_APPLICATION_H
#define DEARENGINE_APPLICATION_H
#include "../engine/core/Scene.h"
#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_init.h"

namespace DA {
    class Application {
    public:
        DE::Scene* current_playing_scene = nullptr;

        /** 应用程序生命周期:启动 */
        bool Start(void *appstate, DE::Scene *scene);
        /** 应用程序生命周期:事件 */
        bool Event(void *appstate, SDL_Event *event);
        /** 应用程序生命周期:每帧逻辑 */
        bool LogicIterate(void *appstate);
        /** 应用程序生命周期:每帧渲染 */
        bool RenderIterate(void *appstate);
        /** 应用程序生命周期:结束 */
        bool End(void *appstate, SDL_AppResult result);

        void SetCurrentPlayingScene(DE::Scene* scene);

        Application() = default;
        ~Application() = default;
    };
} // DA

#endif //DEARENGINE_APPLICATION_H