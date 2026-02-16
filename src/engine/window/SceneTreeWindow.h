//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_SCENETREEWINDOW_H
#define DEARENGINE_SCENETREEWINDOW_H
#include "IEngineWindow.h"
#include "../core/Entity.h"

namespace DE {
    class SceneTreeWindow : public IEngineWindow {
    public:
        SceneTreeWindow();
        /** 窗口标题，用于 ImGui::Begin(title, ...) */
        const char* Title() const override;
        /** 启动 */
        bool Init(void *appstate) override;
        /** 事件 */
        bool Event() override;
        /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
        bool LogicIterate(void *appstate) override;
        /** 每帧渲染，内部应包含处理视口纹理绘制等 */
        bool RenderIterate(void *appstate) override;
        /** 结束 */
        bool Quit() override;
        /** 绘制单个实体 */
        static void DrawEntityNode(DE::Entity* entity);
    };
}

#endif //DEARENGINE_SCENETREEWINDOW_H