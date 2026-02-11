//
// Created by ImSetal on 2026/2/8.
//

#ifndef DEARENGINE_ENTITYCOMPONENTWINDOW_H
#define DEARENGINE_ENTITYCOMPONENTWINDOW_H
#include "IEngineWindow.h"


class EntityComponentWindow : public IEngineWindow {
public:
    EntityComponentWindow();
    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* Title() const override;
    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    void Draw() override;
};


#endif //DEARENGINE_ENTITYCOMPONENTWINDOW_H