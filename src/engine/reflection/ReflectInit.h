//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECTINIT_H
#define DEARENGINE_REFLECTINIT_H
#include "../core/Scene.h"
#include "../core/Entity.h"
#include "../core/component/TestCubeComponent.h"
#include "../core/component/TransformComponent.h"
#include "../core/render_context/ICamera.h"
#include "../serialization/ReflectYamlBuiltins.h"

namespace DE {
    namespace Reflect {
        static void Init() {
            Scene::MakeReflectable();
            Entity::MakeReflectable();
            TransformComponent::MakeReflectable();
            CameraComponent::MakeReflectable();
            TestCubeComponent::MakeReflectable();
            ICamera::MakeReflectable();
            InitReflectYamlSerializers();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
