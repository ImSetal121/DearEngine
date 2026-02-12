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
        Application() = default;
        ~Application() = default;

        /** 引擎生命周期:启动 */
        bool Strat(void *appstate, int argc, char *argv[]);
        /** 引擎生命周期:事件 */
        bool Event(void *appstate, SDL_Event *event);
        /** 引擎生命周期:每帧逻辑 */
        bool LogicIterate(void *appstate);
        /** 引擎生命周期:每帧渲染 */
        bool RenderIterate(void *appstate, SDL_GPUCommandBuffer* command_buffer);
        /** 引擎生命周期:结束 */
        bool Quit(void *appstate, SDL_AppResult result);

    private:
        DE::Scene* first_scene = nullptr;
    };
} // DA

#endif //DEARENGINE_APPLICATION_H