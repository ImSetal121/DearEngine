//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_TRANSFORMCOMPONENT_H
#define DEARENGINE_TRANSFORMCOMPONENT_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

#include "IComponent.h"

namespace DE {
    class TransformComponent : public IComponent {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        const char* GetComponentName() const override;
        bool Start(void *appstate) override;
        bool Iterate(void *appstate) override;

        TransformComponent() = default;
        ~TransformComponent();
    };
} // DE

#endif //DEARENGINE_TRANSFORMCOMPONENT_H