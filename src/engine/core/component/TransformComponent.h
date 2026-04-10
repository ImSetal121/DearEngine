//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_TRANSFORMCOMPONENT_H
#define DEARENGINE_TRANSFORMCOMPONENT_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

#include "IComponent.h"
#include "TransformSpace.h"
#include "../reflection/Reflect.h"
#include "../reflection/Registry.h"

namespace DE {
    class TransformComponent : public IComponent {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        TransformSpace space = ParentSpace;

        glm::vec3 position_world = glm::vec3(0.0f);
        glm::vec3 rotation_world = glm::vec3(0.0f);
        glm::vec3 scale_world = glm::vec3(1.0f);

        void SyncWorldTransform();

        const char* GetComponentName() const override;

        bool EditorStart(void *appstate) override;
        bool Start(void *appstate) override;
        bool Event() override;
        bool EditorIterate(void *appstate) override;
        bool LogicIterate(void *appstate) override;
        bool RenderIterate(void *appstate, RenderContext* render_context) override;
        bool End() override;
        bool Quit() override;

        static void MakeReflectable() {
            DE::Reflect::AddClass<TransformComponent>("TransformComponent")
                .AddMemberVar("position", &TransformComponent::position)
                .AddMemberVar("rotation", &TransformComponent::rotation)
                .AddMemberVar("scale", &TransformComponent::scale)
                .AddMemberVar("space", &TransformComponent::space);
        }

        TransformComponent() = default;
        ~TransformComponent();
    };
} // DE

#endif //DEARENGINE_TRANSFORMCOMPONENT_H