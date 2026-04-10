//
// Created by ImSetal on 2026/2/15.
//

#include "TransformComponent.h"

#include "../Entity.h"

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