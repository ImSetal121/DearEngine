//
// Created by ImSetal on 2026/2/8.
//

#include "SceneTreeWindow.h"
#include "imgui.h"

SceneTreeWindow::SceneTreeWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* SceneTreeWindow::Title() const {
    return "场景树";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void SceneTreeWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    ImGui::End();
}