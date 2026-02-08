//
// Created by ImSetal on 2026/2/8.
//

#include "ConsoleWindow.h"

#include "imgui.h"

ConsoleWindow::ConsoleWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* ConsoleWindow::Title() const {
    return "控制台";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void ConsoleWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    ImGui::End();
}