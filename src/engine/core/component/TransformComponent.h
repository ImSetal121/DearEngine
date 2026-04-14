//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_TRANSFORMCOMPONENT_H
#define DEARENGINE_TRANSFORMCOMPONENT_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "IComponent.h"
#include "TransformSpace.h"
#include "../reflection/Reflect.h"
#include "../reflection/Registry.h"
#include "glm/detail/type_quat.hpp"

namespace DE {
    class TransformComponent : public IComponent {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        TransformSpace space = ParentSpace;

        glm::vec3 position_world = glm::vec3(0.0f);
        glm::quat rotation_world = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // 规范表示，四元数
        glm::vec3 scale_world    = glm::vec3(1.0f);

        void SyncWorldTransform();
        void SetWorldTransform(glm::mat4 transform);
        void SetWorldPosition(glm::vec3 position);
        void SetWorldRotation(glm::vec3 rotation_degrees);
        void SetWorldRotation(glm::quat world_quat);
        void SetWorldScale(glm::vec3 scale);

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