//
// Created by ImSetal on 2026/2/8.
//

#include "EntityComponentWindow.h"

#include <typeindex>

#include "imgui.h"
#include "../Engine.h"

namespace DE {
    class Entity;
}

EntityComponentWindow::EntityComponentWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* EntityComponentWindow::Title() const {
    return "实体组件";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void EntityComponentWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());

    DE::Entity* entity = DE::Engine::GetSelectedEntity();
    if (!entity) {
        ImGui::TextDisabled("(未选中实体 — 在场景树中点击实体)");
        ImGui::End();
        return;
    }

    ImGui::Text("实体: %s", entity->name.c_str());
    ImGui::Separator();

    if (entity->components_.empty()) {
        ImGui::TextDisabled("(无组件)");
    } else {
        for (auto& kv : entity->components_) {
            const std::type_index& typeId = kv.first;
            // typeid.name() 可能返回修饰名，例如 "class DE::TestComponent"
            ImGui::BulletText("%s", typeId.name());
        }
    }

    ImGui::End();
}