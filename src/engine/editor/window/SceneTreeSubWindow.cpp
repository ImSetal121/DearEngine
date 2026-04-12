//
// Created by ImSetal on 2026/2/8.
//

#include "SceneTreeSubWindow.h"
#include "imgui.h"
#include "../EngineEditor.h"
#include "../../../State.h"
#include <algorithm>
#include <optional>

#include "../../core/component/TransformComponent.h"

namespace DE {

    // ---- Drag-drop helpers ----

    // 从当前位置（父节点或场景根）摘出实体，返回所有权
    static std::unique_ptr<Entity> DetachEntity(Scene* scene, Entity* entity) {
        if (entity->parent) {
            return entity->parent->DetachChild(entity);
        }
        auto& roots = scene->root;
        auto it = std::find_if(roots.begin(), roots.end(),
            [entity](const auto& p) { return p.get() == entity; });
        if (it != roots.end()) {
            auto ptr = std::move(*it);
            roots.erase(it);
            return ptr;
        }
        return nullptr;
    }

    // ancestor 是否是 entity 的祖先节点
    static bool IsAncestorOf(Entity* ancestor, Entity* entity) {
        Entity* cur = entity->parent;
        while (cur) {
            if (cur == ancestor) return true;
            cur = cur->parent;
        }
        return false;
    }

    // 在 before 之前插入 entity（同级，共享 before 的父节点）
    static void InsertBefore(Scene* scene, Entity* before, std::unique_ptr<Entity> entity) {
        entity->parent = before->parent;
        if (before->parent) {
            auto& siblings = before->parent->children;
            auto it = std::find_if(siblings.begin(), siblings.end(),
                [before](const auto& p) { return p.get() == before; });
            siblings.insert(it, std::move(entity));
        } else {
            auto& roots = scene->root;
            auto it = std::find_if(roots.begin(), roots.end(),
                [before](const auto& p) { return p.get() == before; });
            roots.insert(it, std::move(entity));
        }
    }

    // 延迟执行的拖拽操作，避免在遍历容器时修改它
    // 插入前解决重名，逻辑与 AddChild 保持一致
    static void ResolveNameConflict(Scene* scene, Entity* newParent, Entity* entity) {
        auto nameExists = [&](const std::string& name) -> bool {
            if (newParent) return newParent->GetChildByName(name) != nullptr;
            for (auto& e : scene->root)
                if (e->name == name) return true;
            return false;
        };

        if (entity->name.empty()) {
            int i = 1;
            while (nameExists("entity " + std::to_string(i))) i++;
            entity->name = "entity " + std::to_string(i);
        } else if (nameExists(entity->name)) {
            std::string original = entity->name;
            int i = 1;
            while (nameExists(original + " " + std::to_string(i))) i++;
            entity->name = original + " " + std::to_string(i);
        }
    }

    static Entity* s_pendingDelete = nullptr;

    struct PendingDrop {
        Entity* dragged;
        enum class Op { Reparent, InsertBefore } op;
        Entity* target;
    };
    static std::optional<PendingDrop> s_pendingDrop;
    Entity* s_pendingParent = nullptr;  // nullptr表示添加到根节点

    // 节点之间的插入区域（零占高：绘制后重置光标，不影响间距）
    static void DrawInsertZone(Entity* before) {
        ImGui::PushID((void*)((uintptr_t)before ^ 0xDEADBEEFu));
        float width = ImGui::GetContentRegionAvail().x;
        float cursorY = ImGui::GetCursorPosY();

        ImGui::InvisibleButton("##gap", ImVec2(width > 0.0f ? width : 1.0f, 6.0f));

        if (ImGui::BeginDragDropTarget()) {
            // 用屏幕坐标在节点顶部画一条蓝线作为插入指示
            ImVec2 winPos = ImGui::GetWindowPos();
            float lineY = winPos.y + cursorY - ImGui::GetScrollY();
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(winPos.x, lineY),
                ImVec2(winPos.x + width, lineY),
                IM_COL32(70, 130, 230, 220), 2.0f);

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                Entity* dragged = *(Entity**)payload->Data;
                if (dragged != before && !IsAncestorOf(dragged, before))
                    s_pendingDrop = { dragged, PendingDrop::Op::InsertBefore, before };
            }
            ImGui::EndDragDropTarget();
        }

        // 重置光标，使该区域不占用垂直空间
        ImGui::SetCursorPosY(cursorY);
        ImGui::PopID();
    }

    // ---- SceneTreeSubWindow methods ----

    SceneTreeSubWindow::SceneTreeSubWindow() = default;

    const char* SceneTreeSubWindow::Title() const {
        return "场景树";
    }

    bool SceneTreeSubWindow::Init(void *appstate) {
        return IEditorSubWindow::Init(appstate);
    }

    bool SceneTreeSubWindow::Event() {
        return IEditorSubWindow::Event();
    }

    bool SceneTreeSubWindow::EditorIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());

        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_editor_subwindow = this;

        DE::Scene* scene = DE::EngineEditor::GetEditingScene();
        if (scene) {
            if (ImGui::Button("+添加实体")) {
                s_pendingParent = nullptr;
                ImGui::OpenPopup("add_component_popup");
            }
            for (auto& entity : scene->root) {
                DrawInsertZone(entity.get());
                DrawEntityNode(scene, entity.get());
            }

            // 树渲染完毕后统一处理删除/拖拽，避免迭代中修改容器
            if (s_pendingDelete) {
                if (DE::EngineEditor::GetSelectedEntity() == s_pendingDelete)
                    DE::EngineEditor::SetSelectedEntity(nullptr);
                DetachEntity(scene, s_pendingDelete); // unique_ptr 离开作用域即销毁
                s_pendingDelete = nullptr;
            }

            if (s_pendingDrop) {
                auto op = *s_pendingDrop;
                s_pendingDrop.reset();
                auto ptr = DetachEntity(scene, op.dragged);
                if (ptr) {
                    if (op.op == PendingDrop::Op::Reparent)
                        op.target->AddChild(std::move(ptr));
                    else {
                        ResolveNameConflict(scene, op.target->parent, ptr.get());
                        InsertBefore(scene, op.target, std::move(ptr));
                    }
                }
            }
        } else {
            ImGui::TextDisabled("(无场景)");
        }


        if (ImGui::BeginPopup("add_component_popup")) {
            std::unique_ptr<Entity> new_entity = nullptr;

            if (ImGui::MenuItem("空实体")) {
                new_entity = std::make_unique<Entity>();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Camera")) {
                new_entity = std::make_unique<Entity>();
                new_entity->AddComponent<TransformComponent>();
                new_entity->AddComponent<CameraComponent>();
                new_entity->name = "Camera";
            }

            if (new_entity) {
                if (s_pendingParent)
                    s_pendingParent->AddChild(std::move(new_entity));
                else
                    scene->AddEntity(std::move(new_entity));
            }

            ImGui::EndPopup();
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

    void SceneTreeSubWindow::DrawEntityNode(Scene* scene, Entity* entity) {
        if (!entity) return;

        ImGui::PushID(entity);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding;
        if (entity->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;
        else
            flags |= ImGuiTreeNodeFlags_OpenOnArrow;

        if (entity == DE::EngineEditor::GetSelectedEntity())
            flags |= ImGuiTreeNodeFlags_Selected;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
        bool opened = ImGui::TreeNodeEx("##node", flags, "%s", entity->name.c_str());
        ImGui::PopStyleVar();

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("添加子实体"))
                // entity->AddChild(std::make_unique<Entity>());
                s_pendingParent = entity;
                ImGui::OpenPopup("add_component_popup");
            if (ImGui::MenuItem("删除此实体")) {
                s_pendingDelete = entity;
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemClicked())
            DE::EngineEditor::SetSelectedEntity(entity);

        // 拖拽源
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(Entity*));
            ImGui::Text("移动: %s", entity->name.c_str());
            ImGui::EndDragDropSource();
        }

        // 拖放目标：拖到节点上 = 成为该节点最后一个子节点
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY")) {
                Entity* dragged = *(Entity**)payload->Data;
                if (dragged != entity && !IsAncestorOf(dragged, entity))
                    s_pendingDrop = { dragged, PendingDrop::Op::Reparent, entity };
            }
            ImGui::EndDragDropTarget();
        }

        if (opened) {
            for (auto& child : entity->children) {
                DrawInsertZone(child.get());
                DrawEntityNode(scene, child.get());
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}
