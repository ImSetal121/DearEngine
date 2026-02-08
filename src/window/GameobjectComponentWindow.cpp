//
// Created by ImSetal on 2026/2/8.
//

#include "GameobjectComponentWindow.h"
#include "imgui.h"

GameobjectComponentWindow::GameobjectComponentWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* GameobjectComponentWindow::Title() const {
    return "GameObject组件";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void GameobjectComponentWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    ImGui::End();
}