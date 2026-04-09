//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECTINIT_H
#define DEARENGINE_REFLECTINIT_H
#include "../Scene.h"
#include "../Entity.h"
#include "../component/CameraComponent.h"
#include "../component/DirLightComponent.h"
#include "../component/TestCubeComponent.h"
#include "../component/TransformComponent.h"
#include "../render_context/ICamera.h"
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
            DE_REGISTER_SCENE_COMPONENT(DirLightComponent, "DirLightComponent");
            InitReflectYamlSerializers();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
