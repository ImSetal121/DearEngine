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
                // 属性编辑区域
                Reflect::TypeDescriptor type_descriptor = Reflect::GetByName(Reflect::GetTypeName(typeId)->c_str());
                for (auto& member_var : type_descriptor.member_vars()) {
                    // 通用渲染
                    if (member_var.type() == std::type_index(typeid(glm::vec3))) {
                        glm::vec3 val = std::any_cast<glm::vec3>(member_var.GetValueAny(type_descriptor.WrapObject(kv.second.get())));
                        if (ImGui::DragFloat3(member_var.name().c_str(), &val.x)) {
                            member_var.SetValueAny(type_descriptor.WrapMutableObject(kv.second.get()), val);
                        }
                    }
                }
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