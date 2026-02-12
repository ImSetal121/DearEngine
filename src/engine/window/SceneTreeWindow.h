//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_SCENETREEWINDOW_H
#define DEARENGINE_SCENETREEWINDOW_H
#include "IEngineWindow.h"
#include "../core/Entity.h"


class SceneTreeWindow : public IEngineWindow {
public:
    SceneTreeWindow();
    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* Title() const override;
    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    void Draw() override;
    /** 绘制单个实体 */
    static void DrawEntityNode(DE::Entity* entity);
};


#endif //DEARENGINE_SCENETREEWINDOW_H