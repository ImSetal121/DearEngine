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
#include "../component/ColliderComponent.h"
#include "../component/RigidBodyComponent.h"
#include "../render_context/ICamera.h"
#include "../serialization/ReflectYamlBuiltins.h"
#include "ComponentFactory.h"

namespace DE {
    namespace Reflect {
        static void Init() {
            // 基本类型注册
            Scene::MakeReflectable();
            Entity::MakeReflectable();
            ICamera::MakeReflectable();
            // 组件注册
            DE_REGISTER_SCENE_COMPONENT(TransformComponent, "TransformComponent");
            DE_REGISTER_SCENE_COMPONENT(CameraComponent, "CameraComponent");
            DE_REGISTER_SCENE_COMPONENT(TestCubeComponent, "TestCubeComponent");
            DE_REGISTER_SCENE_COMPONENT(DirLightComponent, "DirLightComponent");
            // DE_REGISTER_SCENE_COMPONENT(ColliderComponent, "ColliderComponent");
            DE_REGISTER_SCENE_COMPONENT(RigidBodyComponent, "RigidBodyComponent");
            InitReflectYamlSerializers();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
