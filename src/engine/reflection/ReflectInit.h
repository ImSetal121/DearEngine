//
// Created by ImSetal on 2026/3/10.
//

#ifndef DEARENGINE_REFLECTINIT_H
#define DEARENGINE_REFLECTINIT_H
#include "../core/component/TransformComponent.h"

namespace DE {
    namespace Reflect {
        static void Init() {
            TransformComponent::MakeReflectable();
        }
    }
}

#endif //DEARENGINE_REFLECTINIT_H
