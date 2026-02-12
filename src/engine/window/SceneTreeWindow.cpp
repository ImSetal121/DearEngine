//
// Created by ImSetal on 2026/2/8.
//

#include "SceneTreeWindow.h"
#include "imgui.h"
#include "../Engine.h"

namespace DE {
    class Scene;
}

SceneTreeWindow::SceneTreeWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* SceneTreeWindow::Title() const {
    return "场景树";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void SceneTreeWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());

    DE::Scene* scene = DE::Engine::GetEditingScene();
    if (scene) {
        for (auto& entity : scene->root_) {
            DrawEntityNode(entity.get());
        }
    } else {
        ImGui::TextDisabled("(无场景)");
    }

    ImGui::End();
}

void SceneTreeWindow::DrawEntityNode(DE::Entity *entity) {
    if (!entity) return;

    // 叶子节点用 ImGuiTreeNodeFlags_Leaf，有子节点则用默认
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (entity->children_.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }else {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
    }

    // 处理选中项
    if (entity == DE::Engine::GetSelectedEntity()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    flags |= ImGuiTreeNodeFlags_FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3.2));    // 行高
    bool opened = ImGui::TreeNodeEx(entity->name.c_str(), flags);

    ImGui::PopStyleVar();

    if (ImGui::IsItemClicked()) {
        DE::Engine::SetSelectedEntity(entity);
    }

    if (opened) {
        for (auto& child : entity->children_) {
            DrawEntityNode(child.get());
        }
        ImGui::TreePop();
    }
}
