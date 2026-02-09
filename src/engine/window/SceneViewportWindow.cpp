//
// Created by ImSetal on 2026/2/8.
//

#include "SceneViewportWindow.h"
#include "imgui.h"

SceneViewportWindow::SceneViewportWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* SceneViewportWindow::Title() const {
    return "场景视口";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void SceneViewportWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    ImGui::End();
}