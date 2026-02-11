//
// Created by ImSetal on 2026/2/8.
//

#include "EntityComponentWindow.h"
#include "imgui.h"

EntityComponentWindow::EntityComponentWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* EntityComponentWindow::Title() const {
    return "GameObject组件";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void EntityComponentWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());
    ImGui::End();
}