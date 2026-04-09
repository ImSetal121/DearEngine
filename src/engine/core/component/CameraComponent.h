//
// Created by ImSetal on 2026/3/4.
//

#ifndef DEARENGINE_CAMERACOMPOENT_H
#define DEARENGINE_CAMERACOMPOENT_H
#include "IComponent.h"
#include "../reflection/Reflect.h"

namespace DE {
    class CameraComponent : public IComponent{
        public:
        const char* GetComponentName() const override;
        ICamera* camera = nullptr;

        bool EditorStart(void *appstate) override;
        bool Start(void *appstate) override;

        CameraComponent() = default;

        static void MakeReflectable() {
            DE::Reflect::AddClass<CameraComponent>("CameraComponent")
                .AddMemberVar("camera", &CameraComponent::camera);
        }
    };
} // DE

#endif //DEARENGINE_CAMERACOMPOENT_H