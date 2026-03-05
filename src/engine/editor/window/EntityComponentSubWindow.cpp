//
// Created by ImSetal on 2026/2/8.
//

#include "EntityComponentSubWindow.h"

#include <typeindex>

#include "imgui.h"
#include "../EngineEditor.h"
#include "../../../State.h"

namespace DE {
    EntityComponentSubWindow::EntityComponentSubWindow() = default;

    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* EntityComponentSubWindow::Title() const {
        return "实体组件";
    }

    bool EntityComponentSubWindow::Init(void *appstate) {
        return IEditorSubWindow::Init(appstate);
    }

    bool EntityComponentSubWindow::Event() {
        return IEditorSubWindow::Event();
    }

    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    bool EntityComponentSubWindow::LogicIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_editor_subwindow = this;

        DE::Entity* entity = DE::EngineEditor::GetSelectedEntity();
        if (!entity) {
            ImGui::TextDisabled("(未选中实体 — 在场景树中点击实体)");
            ImGui::End();
            return false;
        }

        ImGui::Text("实体: %s", entity->name.c_str());
        ImGui::Separator();

        if (entity->components.empty()) {
            ImGui::TextDisabled("(无组件)");
        } else {
            for (auto& kv : entity->components) {
                const std::type_index& typeId = kv.first;
                // typeid.name() 可能返回修饰名，例如 "class DE::TestComponent"
                ImGui::BulletText("%s", kv.second->GetComponentName());
            }
        }

        ImGui::End();

        return true;
    }

    bool EntityComponentSubWindow::RenderIterate(void *appstate) {
        return IEditorSubWindow::RenderIterate(appstate);
    }

    bool EntityComponentSubWindow::Quit() {
        return IEditorSubWindow::Quit();
    }
}