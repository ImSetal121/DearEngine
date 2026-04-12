//
// Created by ImSetal on 2026/2/8.
//

#include "EntityComponentSubWindow.h"

#include <typeindex>

#include "imgui.h"
#include "../EngineEditor.h"
#include "../../../State.h"
#include "../../core/component/TransformSpace.h"
#include "../../core/reflection/ComponentFactory.h"

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
    bool EntityComponentSubWindow::EditorIterate(void *appstate) {
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

        // ImGui::Text("实体: %s", entity->name.c_str());
        char buf[256];
        strncpy(buf, entity->name.c_str(), sizeof(buf));
        if (ImGui::InputText("##name", buf, sizeof(buf)))
            entity->name = buf;

        if (ImGui::IsItemDeactivatedAfterEdit()) {
            std::string desired = entity->name;
            entity->name = "";  // 临时清除，避免 GetChildByName 找到自身

            auto nameExists = [&](const std::string& name) -> bool {
                if (entity->parent)
                    return entity->parent->GetChildByName(name) != nullptr;
                DE::Scene* scene = DE::EngineEditor::GetEditingScene();
                if (!scene) return false;
                for (auto& e : scene->root)
                    if (e->name == name) return true;
                return false;
            };

            if (desired.empty()) {
                int i = 1;
                while (nameExists("entity " + std::to_string(i))) i++;
                entity->name = "entity " + std::to_string(i);
            } else if (nameExists(desired)) {
                int i = 1;
                while (nameExists(desired + " " + std::to_string(i))) i++;
                entity->name = desired + " " + std::to_string(i);
            } else {
                entity->name = desired;
            }
        }
        ImGui::Separator();

        std::type_index pendingRemove = typeid(void);

        if (entity->components.empty()) {
            ImGui::TextDisabled("(无组件)");
        } else {
            for (auto& kv : entity->components) {
                ImGui::PushID(kv.second.get());
                const std::type_index& typeId = kv.first;
                ImGui::BulletText("%s", kv.second->GetComponentName());
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 80);
                if (ImGui::SmallButton("删除此组件")) {
                    pendingRemove = typeId;
                }
                // 属性编辑区域
                Reflect::TypeDescriptor type_descriptor = Reflect::GetByName(Reflect::GetTypeName(typeId)->c_str());
                for (auto& member_var : type_descriptor.member_vars()) {
                    // 通用渲染
                    if (member_var.type() == std::type_index(typeid(glm::vec3))) {
                        auto val = std::any_cast<glm::vec3>(member_var.GetValueAny(type_descriptor.WrapObject(kv.second.get())));
                        if (ImGui::DragFloat3(member_var.name().c_str(), &val.x, 0.125f)) {
                            member_var.SetValueAny(type_descriptor.WrapMutableObject(kv.second.get()), val);
                        }
                    }

                    if (member_var.type() == std::type_index(typeid(TransformSpace))) {
                        auto val = std::any_cast<TransformSpace>(member_var.GetValueAny(type_descriptor.WrapObject(kv.second.get())));
                        const char* items[] = { "继承父实体", "世界空间"};
                        int current = (int)val;
                        if (ImGui::Combo(member_var.name().c_str(), &current, items, 2)) {
                            auto new_val = static_cast<TransformSpace>(current);
                            member_var.SetValueAny(type_descriptor.WrapMutableObject(kv.second.get()), new_val);
                        }
                    }

                    if (member_var.type() == std::type_index(typeid(IDirLight*))) {
                        auto val = std::any_cast<IDirLight*>(member_var.GetValueAny(type_descriptor.WrapObject(kv.second.get())));
                        if (ImGui::ColorEdit3("环境光颜色", &val->ambient.x)) {}
                        if (ImGui::ColorEdit3("漫反射颜色", &val->diffuse.x)) {}
                        if (ImGui::ColorEdit3("高光颜色", &val->specular.x)) {}
                    }
                }
                ImGui::PopID();
            }
        }

        if (pendingRemove != typeid(void))
            entity->components.erase(pendingRemove);

        ImGui::SetCursorPosX(ImGui::GetWindowWidth()/2 - 32);
        if (ImGui::Button("添加组件")) {
            ImGui::OpenPopup("add_component_popup");
        }
        if (ImGui::BeginPopup("add_component_popup")) {
            for (auto& component_map : g_componentFactories) {
                if (ImGui::MenuItem(component_map.first.c_str())) {
                    entity->AddComponent(g_componentTypeIndex.at(component_map.first), std::move(component_map.second()));
                }
            }
            ImGui::EndPopup();
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
