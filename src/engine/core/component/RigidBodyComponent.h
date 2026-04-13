//
// Created by ImSetal on 2026/4/13.
//

#ifndef DEARENGINE_RIGIDBODYCOMPONENT_H
#define DEARENGINE_RIGIDBODYCOMPONENT_H
#include "IComponent.h"

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyID.h"

namespace DE {

class RigidBodyComponent : public IComponent {
    public:
    std::function<void(const JPH::BodyID& other)> on_contact_added;
    std::function<void(const JPH::BodyID& other)> on_contact_persisted;
    std::function<void(const JPH::BodyID& other)> on_contact_removed;
    const char* GetComponentName() const override;

    RigidBodyComponent() = default;
    static void MakeReflectable() {
        DE::Reflect::AddClass<RigidBodyComponent>("RigidBodyComponent");
    }
    private:
    JPH::BodyID body_id_;
};

} // DE

#endif //DEARENGINE_RIGIDBODYCOMPONENT_H
