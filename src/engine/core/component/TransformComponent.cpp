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
    
    void TransformComponent::SyncWorldTransform() {
        if (space == ParentSpace) {
            position_world = AddParentPosition(GetOwner());
        }else {
            position_world = position;
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

    bool TransformComponent::LogicIterate(void *appstate) {
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