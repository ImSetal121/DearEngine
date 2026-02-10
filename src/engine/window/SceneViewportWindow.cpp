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
    if (viewport_texture_ && *viewport_width_ > 0 && *viewport_height_ > 0) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        *viewport_width_ = avail.x;
        *viewport_height_ = avail.y;
        ImGui::Image(viewport_texture_, ImVec2(*viewport_width_, *viewport_height_));
    }
    ImGui::End();
}

/** 设置视口绘制纹理指针 */
void SceneViewportWindow::SetViewportTexture(void *texture, int* width, int* height) {
    viewport_texture_ = texture;
    viewport_width_ = width;
    viewport_height_ = height;
}
