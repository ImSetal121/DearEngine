//
// Created by ImSetal on 2026/2/15.
//

#include "TransformComponent.h"

#include "../Entity.h"
#include <glm/gtc/quaternion.hpp>

namespace DE {

    // 递归计算世界空间旋转四元数（父链乘法）
    static glm::quat ComputeWorldRotationQuat(Entity* e) {
        auto* tc = e->GetComponent<TransformComponent>();
        glm::quat local = glm::quat(glm::radians(tc->rotation));
        if (tc->space == ParentSpace && e->parent && e->parent->HasComponent<TransformComponent>()) {
            return ComputeWorldRotationQuat(e->parent) * local;
        }
        return local;
    }

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
            rotation_world = ComputeWorldRotationQuat(GetOwner());
            scale_world    = AddParentScale(GetOwner());
        } else {
            position_world = position;
            rotation_world = glm::quat(glm::radians(rotation));
            scale_world    = scale;
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

    // 从欧拉角（角度）设置世界旋转，同时反推本地 rotation
    void TransformComponent::SetWorldRotation(glm::vec3 rotation_degrees) {
        glm::quat world_quat = glm::quat(glm::radians(rotation_degrees));
        SetWorldRotation(world_quat);
        // 同步 euler 显示值（editor 用）
        rotation = glm::degrees(glm::eulerAngles(rotation_world));
    }

    // 直接设置世界旋转四元数，同时反推本地 rotation euler
    void TransformComponent::SetWorldRotation(glm::quat world_quat) {
        if (space == ParentSpace && GetOwner()->parent && GetOwner()->parent->HasComponent<TransformComponent>()) {
            glm::quat parent_quat = ComputeWorldRotationQuat(GetOwner()->parent);
            glm::quat local_quat  = glm::inverse(parent_quat) * world_quat;
            rotation = glm::degrees(glm::eulerAngles(local_quat));
        } else {
            rotation = glm::degrees(glm::eulerAngles(world_quat));
        }
        rotation_world = world_quat;
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
        glm::vec3 world_pos = glm::vec3(transform[3]);
        glm::vec3 world_scale(
            glm::length(glm::vec3(transform[0])),
            glm::length(glm::vec3(transform[1])),
            glm::length(glm::vec3(transform[2]))
        );
        glm::mat3 rot_mat(
            glm::vec3(transform[0]) / world_scale.x,
            glm::vec3(transform[1]) / world_scale.y,
            glm::vec3(transform[2]) / world_scale.z
        );
        SetWorldPosition(world_pos);
        SetWorldRotation(glm::quat_cast(rot_mat));
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