//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECTINIT_H
#define DEARENGINE_REFLECTINIT_H
#include "../core/component/TransformComponent.h"
#include "../serialization/ReflectYaml.h"

namespace DE {
    namespace Reflect {
        static void Init() {
            TransformComponent::MakeReflectable();
            InitReflectYamlSerializers();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
