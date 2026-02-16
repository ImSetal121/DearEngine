//
// Created by ImSetal on 2026/2/15.
//

#include "TransformComponent.h"

namespace DE {
    const char * TransformComponent::GetComponentName() const {
        return "变换";
    }

    bool TransformComponent::Start(void *appstate) {
        return IComponent::Start(appstate);
    }

    bool TransformComponent::Iterate(void *appstate) {
        return IComponent::Iterate(appstate);
    }

    TransformComponent::~TransformComponent() = default;
} // DE