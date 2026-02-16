//
// Created by ImSetal on 2026/2/15.
//

#include "TransformComponent.h"

namespace DE {
    const char * TransformComponent::GetComponentName() const {
        return "变换";
    }

    bool TransformComponent::Init(void *appstate) {
        return IComponent::Init(appstate);
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

    bool TransformComponent::Quit() {
        return IComponent::Quit();
    }

    TransformComponent::~TransformComponent() = default;
} // DE