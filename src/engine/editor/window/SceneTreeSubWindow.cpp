//
// Created by ImSetal on 2026/2/8.
//

#include "SceneTreeSubWindow.h"
#include "imgui.h"
#include "../EngineEditor.h"
#include "../../../State.h"

namespace DE {
    SceneTreeSubWindow::SceneTreeSubWindow() = default;

    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* SceneTreeSubWindow::Title() const {
        return "场景树";
    }

    bool SceneTreeSubWindow::Init(void *appstate) {
        return IEditorSubWindow::Init(appstate);
    }

    bool SceneTreeSubWindow::Event() {
        return IEditorSubWindow::Event();
    }

    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    bool SceneTreeSubWindow::LogicIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_editor_subwindow = this;

        DE::Scene* scene = DE::EngineEditor::GetEditingScene();
        if (scene) {
            for (auto& entity : scene->root) {
                DrawEntityNode(entity.get());
            }
        } else {
            ImGui::TextDisabled("(无场景)");
        }

        ImGui::End();

        return true;
    }

    bool SceneTreeSubWindow::RenderIterate(void *appstate) {
        return IEditorSubWindow::RenderIterate(appstate);
    }

    bool SceneTreeSubWindow::Quit() {
        return IEditorSubWindow::Quit();
    }

    void SceneTreeSubWindow::DrawEntityNode(DE::Entity *entity) {
        if (!entity) return;

        // 叶子节点用 ImGuiTreeNodeFlags_Leaf，有子节点则用默认
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
        if (entity->children.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }else {
            flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        }

        // 处理选中项
        if (entity == DE::EngineEditor::GetSelectedEntity()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        flags |= ImGuiTreeNodeFlags_FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));    // 行高
        bool opened = ImGui::TreeNodeEx(entity->name.c_str(), flags);

        ImGui::PopStyleVar();

        if (ImGui::IsItemClicked()) {
            DE::EngineEditor::SetSelectedEntity(entity);
        }

        if (opened) {
            for (auto& child : entity->children) {
                DrawEntityNode(child.get());
            }
            ImGui::TreePop();
        }
    }
}
