//
// Created by ImSetal on 2026/4/13.
//

#include "PhysicsContactListener.h"
#include "../component/RigidBodyComponent.h"

namespace DE {
    void PhysicsContactListener::OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) {
        std::cout << "新增了一个接触" << std::endl;
        auto* comp1 = Physics::Instance().GetComponent(inBody1.GetID());
        auto* comp2 = Physics::Instance().GetComponent(inBody2.GetID());
        if (comp1 && comp1->on_contact_added)
            comp1->on_contact_added(inBody2.GetID());
        if (comp2 && comp2->on_contact_added)
            comp2->on_contact_added(inBody1.GetID());
    }

    void PhysicsContactListener::OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) {
        std::cout << "一个接触持续存在" << std::endl;
        auto* comp1 = Physics::Instance().GetComponent(inBody1.GetID());
        auto* comp2 = Physics::Instance().GetComponent(inBody2.GetID());
        if (comp1 && comp1->on_contact_added)
            comp1->on_contact_added(inBody2.GetID());
        if (comp2 && comp2->on_contact_added)
            comp2->on_contact_added(inBody1.GetID());
    }

    void PhysicsContactListener::OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) {
        std::cout << "移除了一个接触" << std::endl;
        auto* comp1 = Physics::Instance().GetComponent(inSubShapePair.GetBody1ID());
        auto* comp2 = Physics::Instance().GetComponent(inSubShapePair.GetBody2ID());
        if (comp1 && comp1->on_contact_added)
            comp1->on_contact_added(inSubShapePair.GetBody2ID());
        if (comp2 && comp2->on_contact_added)
            comp2->on_contact_added(inSubShapePair.GetBody1ID());
    }
} // DE