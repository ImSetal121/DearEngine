//
// Created by ImSetal on 2026/2/15.
//

#include "TransformComponent.h"

#include "../Entity.h"
#include <glm/gtc/quaternion.hpp>

namespace DE {
    glm::vec3 AddParentPosition(Entity* e) {
        if (e->HasComponent<TransformComponent>() && e->GetComponent<TransformComponent>()->space == ParentSpace) {
            if (e->parent && e->parent->HasComponent<TransformComponent>()) {
                return AddParentPosition(e->parent) + e->GetComponent<TransformComponent>()->position;
            }else {
                return e->GetComponent<TransformComponent>()->position;
            }
        }
        return e->GetComponent<TransformComponent>()->position;
    }

    glm::vec3 AddParentRotation(Entity* e) {
        if (e->HasComponent<TransformComponent>() && e->GetComponent<TransformComponent>()->space == ParentSpace) {
            if (e->parent && e->parent->HasComponent<TransformComponent>()) {
                return AddParentRotation(e->parent) + e->GetComponent<TransformComponent>()->rotation;
            }else {
                return e->GetComponent<TransformComponent>()->rotation;
            }
        }
        return e->GetComponent<TransformComponent>()->rotation;
    }

    glm::vec3 AddParentScale(Entity* e) {
        if (e->HasComponent<TransformComponent>() && e->GetComponent<TransformComponent>()->space == ParentSpace) {
            if (e->parent && e->parent->HasComponent<TransformComponent>()) {
                return AddParentScale(e->parent) * e->GetComponent<TransformComponent>()->scale;
            }else {
                return e->GetComponent<TransformComponent>()->scale;
            }
        }
        return e->GetComponent<TransformComponent>()->scale;
    }
    
    void TransformComponent::SyncWorldTransform() {
        if (space == ParentSpace) {
            position_world = AddParentPosition(GetOwner());
            rotation_world = AddParentRotation(GetOwner());
            scale_world = AddParentScale(GetOwner());
        }else {
            position_world = position;
            rotation_world = rotation;
            scale_world = scale;
        }
    }

    void TransformComponent::SetWorldPosition(glm::vec3 pos) {
        if (space == ParentSpace && GetOwner()->parent && GetOwner()->parent->HasComponent<TransformComponent>()) {
            position = pos - AddParentPosition(GetOwner()->parent);
        } else {
            position = pos;
        }
        position_world = pos;
    }

    void TransformComponent::SetWorldRotation(glm::vec3 rot) {
        if (space == ParentSpace && GetOwner()->parent && GetOwner()->parent->HasComponent<TransformComponent>()) {
            rotation = rot - AddParentRotation(GetOwner()->parent);
        } else {
            rotation = rot;
        }
        rotation_world = rot;
    }

    void TransformComponent::SetWorldRotation(glm::quat rotation) {
        rotation_quat_world = rotation;
    }

    void TransformComponent::SetWorldScale(glm::vec3 s) {
        if (space == ParentSpace && GetOwner()->parent && GetOwner()->parent->HasComponent<TransformComponent>()) {
            glm::vec3 parent_scale = AddParentScale(GetOwner()->parent);
            scale = s / parent_scale;
        } else {
            scale = s;
        }
        scale_world = s;
    }

    void TransformComponent::SetWorldTransform(glm::mat4 transform) {
        // 提取世界位置（第 4 列）
        glm::vec3 world_pos = glm::vec3(transform[3]);

        // 提取世界缩放（各列的长度）
        glm::vec3 world_scale(
            glm::length(glm::vec3(transform[0])),
            glm::length(glm::vec3(transform[1])),
            glm::length(glm::vec3(transform[2]))
        );

        // 提取世界旋转（归一化后转四元数再转欧拉角，单位：角度）
        glm::mat3 rot_mat(
            glm::vec3(transform[0]) / world_scale.x,
            glm::vec3(transform[1]) / world_scale.y,
            glm::vec3(transform[2]) / world_scale.z
        );
        glm::vec3 world_rot = glm::degrees(glm::eulerAngles(glm::quat_cast(rot_mat)));

        SetWorldPosition(world_pos);
        SetWorldRotation(world_rot);
        SetWorldScale(world_scale);
    }

    const char * TransformComponent::GetComponentName() const {
        return "变换";
    }

    bool TransformComponent::EditorStart(void *appstate) {
        return IComponent::EditorStart(appstate);
    }

    bool TransformComponent::Start(void *appstate) {
        return IComponent::Start(appstate);
    }

    bool TransformComponent::Event() {
        return IComponent::Event();
    }

    bool TransformComponent::EditorIterate(void *appstate) {
        LogicIterate(appstate);
        return IComponent::EditorIterate(appstate);
    }

    bool TransformComponent::LogicIterate(void *appstate) {
        SyncWorldTransform();
        return IComponent::LogicIterate(appstate);
    }

    bool TransformComponent::RenderIterate(void *appstate, RenderContext* render_context) {
        return IComponent::RenderIterate(appstate, render_context);
    }

    bool TransformComponent::End() {
        return IComponent::End();
    }

    bool TransformComponent::Quit() {
        return IComponent::Quit();
    }

    TransformComponent::~TransformComponent() = default;
} // DE