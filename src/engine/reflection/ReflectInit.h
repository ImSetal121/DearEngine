//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECTINIT_H
#define DEARENGINE_REFLECTINIT_H
#include "../core/Scene.h"
#include "../core/Entity.h"
#include "../core/component/CameraComponent.h"
#include "../core/component/TestCubeComponent.h"
#include "../core/component/TransformComponent.h"
#include "../core/render_context/ICamera.h"
#include "../serialization/ReflectYamlBuiltins.h"
#include "../serialization/SceneSerialization.h"

namespace DE {
    namespace Reflect {
        static void Init() {
            Scene::MakeReflectable();
            Entity::MakeReflectable();
            ICamera::MakeReflectable();
            DE_REGISTER_SCENE_COMPONENT(TransformComponent, "TransformComponent");
            DE_REGISTER_SCENE_COMPONENT(CameraComponent, "CameraComponent");
            DE_REGISTER_SCENE_COMPONENT(TestCubeComponent, "TestCubeComponent");
            InitReflectYamlSerializers();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
