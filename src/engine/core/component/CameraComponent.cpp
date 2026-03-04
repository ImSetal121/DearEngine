//
// Created by ImSetal on 2026/3/4.
//

#include "CameraComponent.h"

#include "TransformComponent.h"
#include "../Entity.h"
#include "../Log.h"

namespace DE {
    const char * CameraComponent::GetComponentName() const {
        return "Camera";
    }

    bool CameraComponent::Start(void *appstate) {
        auto transform = GetOwner()->GetComponent<TransformComponent>();
        if (transform == nullptr)
            DE::Log::Warning("["+GetOwner()->name+"]["+ CameraComponent::GetComponentName()+"]需要Transform组件才能够正常工作.");
        else {
            camera = new ICamera(&transform->position, &transform->rotation);
        }
        return IComponent::Start(appstate);
    }
} // DE