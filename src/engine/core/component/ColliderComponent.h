//
// Created by ImSetal on 2026/4/13.
//

#ifndef DEARENGINE_COLLIDERCOMPONENT_H
#define DEARENGINE_COLLIDERCOMPONENT_H
#include "IComponent.h"

namespace DE {

class ColliderComponent : public IComponent {
    public:
    const char* GetComponentName() const override;

    ColliderComponent() = default;
    static void MakeReflectable() {
        DE::Reflect::AddClass<ColliderComponent>("ColliderComponent");
    }
};

} // DE

#endif //DEARENGINE_COLLIDERCOMPONENT_H
