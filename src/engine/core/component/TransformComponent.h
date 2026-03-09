//
// Created by ImSetal on 2026/2/15.
//

#ifndef DEARENGINE_TRANSFORMCOMPONENT_H
#define DEARENGINE_TRANSFORMCOMPONENT_H
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include "../../reflection//YamlConverters.h"
#include "../../reflection//DeReflectionYaml.h"

#include "IComponent.h"
#include "TransformSpace.h"

namespace DE {
    class TransformComponent : public IComponent {
    public:
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        TransformSpace space = ParentSpace;

        const char* GetComponentName() const override;

        bool EditorStart(void *appstate) override;
        bool Start(void *appstate) override;
        bool Event() override;
        bool LogicIterate(void *appstate) override;
        bool RenderIterate(void *appstate, RenderContext* render_context) override;
        bool End() override;
        bool Quit() override;

        TransformComponent() = default;
        ~TransformComponent();

        DE_REFLECT(TransformComponent,
            DE_FIELD(position),
            DE_FIELD(rotation),
            DE_FIELD(scale),
            DE_FIELD(space)
        )
    };
} // DE

#endif //DEARENGINE_TRANSFORMCOMPONENT_H