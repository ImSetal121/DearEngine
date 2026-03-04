//
// Created by ImSetal on 2026/3/4.
//

#ifndef DEARENGINE_CAMERACOMPOENT_H
#define DEARENGINE_CAMERACOMPOENT_H
#include "IComponent.h"

namespace DE {
    class CameraComponent : public IComponent{
        public:
        const char* GetComponentName() const override;
        ICamera* camera = nullptr;

        bool Start(void *appstate) override;

        CameraComponent() = default;
    };
} // DE

#endif //DEARENGINE_CAMERACOMPOENT_H