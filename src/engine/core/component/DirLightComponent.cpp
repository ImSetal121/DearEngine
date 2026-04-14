//
// Created by ImSetal on 2026/4/9.
//

#include "DirLightComponent.h"
#include "TransformComponent.h"
#include "../Entity.h"
#include "../Log.h"

namespace DE {
    const char * DirLightComponent::GetComponentName() const {
        return "定向光组件";
    }

    bool DirLightComponent::EditorStart(void *appstate) {
        Start(appstate);
        return IComponent::EditorStart(appstate);
    }

    bool DirLightComponent::Start(void *appstate) {
        dir_light = new IDirLight(glm::vec3(0.0f));
        return IComponent::Start(appstate);
    }

    bool DirLightComponent::RenderIterate(void *appstate, RenderContext *render_context) {
        if (dir_light) {
            auto transform = GetOwner()->GetComponent<TransformComponent>();
            if (transform == nullptr)
                DE::Log::Warning("["+GetOwner()->name+"]["+ DirLightComponent::GetComponentName()+"]需要Transform组件才能够正常工作.");
            else {
                glm::mat4 rotMat = glm::mat4_cast(transform->rotation_world);
                glm::vec3 direction = glm::normalize(glm::vec3(rotMat * glm::vec4(0, -1, 0, 0)));
                dir_light->direction = direction;
            }

            if (render_context) {
                if (render_context->dirLight == nullptr) {
                    render_context->dirLight = dir_light;
                }
            }
        }
        return IComponent::RenderIterate(appstate, render_context);
    }
} // DE