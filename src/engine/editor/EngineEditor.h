//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ENGINE_H
#define DEARENGINE_ENGINE_H

#include "../core/Scene.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"

namespace DE {
    class EngineEditor {
    public:
        EngineEditor() = default;

        /** 引擎生命周期:启动 */
        static bool Init(void *appstate, int argc, char *argv[]);
        /** 引擎生命周期:事件 */
        static bool Event(void *appstate, SDL_Event *event);
        /** 引擎生命周期:每帧逻辑 */
        static bool LogicIterate(void *appstate);
        /** 引擎生命周期:每帧渲染 */
        static bool RenderIterate(void *appstate);
        /** 引擎生命周期:结束 */
        static bool Quit(void *appstate, SDL_AppResult result);

        static Scene* GetEditingScene();
        static void SetEditingScene(Scene* scene);
        static Entity* GetSelectedEntity();
        static void SetSelectedEntity(Entity* entity);

    };
}

#endif //DEARENGINE_ENGINE_H