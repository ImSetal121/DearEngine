//
// Created by ImSetal on 2026/2/22.
//

#ifndef DEARENGINE_ASSETMANAGERWINDOW_H
#define DEARENGINE_ASSETMANAGERWINDOW_H
#include "IEngineWindow.h"

namespace DE {
    class AssetManagerWindow : public IEngineWindow {
    public:
        AssetManagerWindow();
        /** 窗口标题，用于 ImGui::Begin(title, ...) */
        const char *Title() const override {return "资产管理器";};
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
    };
} // DE

#endif //DEARENGINE_ASSETMANAGERWINDOW_H