//
// Created by ImSetal on 2026/2/8.
//

#ifndef LEARNCPP_CONSOLEWINDOW_H
#define LEARNCPP_CONSOLEWINDOW_H

#include "IEngineWindow.h"

class ConsoleWindow : public IEngineWindow {
    public:
    ConsoleWindow();
    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* Title() const override;
    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    void Draw() override;
};


#endif //LEARNCPP_CONSOLEWINDOW_H