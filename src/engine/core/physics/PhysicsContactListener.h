//
// Created by ImSetal on 2026/4/13.
//

#ifndef DEARENGINE_DECONTACTLISTENER_H
#define DEARENGINE_DECONTACTLISTENER_H

#include <iostream>
#include <Jolt/Jolt.h>

#include "Physics.h"
#include "Jolt/Physics/Collision/ContactListener.h"

namespace DE {

class PhysicsContactListener : public JPH::ContactListener
    {
    public:
        // OnContactValidate：宽相检测到两个对象可能碰撞后，在真正计算接触之前调用。
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2,
                                                      JPH::RVec3Arg inBaseOffset,
                                                      const JPH::CollideShapeResult &inCollisionResult) override
        {
            std::cout << "接触验证回调" << std::endl;
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair; // 允许碰撞
        }

        // OnContactAdded：两个刚体刚刚开始接触（本帧新产生的接触点）。
        // ContactManifold 包含接触点坐标、法线、穿透深度等详细信息。
        // ioSettings 可以修改本次接触的弹性/摩擦参数。
        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2,
                                    const JPH::ContactManifold &inManifold,
                                    JPH::ContactSettings &ioSettings) override;

        // OnContactPersisted：两个刚体持续接触（上帧也有、本帧还在）。
        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2,
                                        const JPH::ContactManifold &inManifold,
                                        JPH::ContactSettings &ioSettings) override;

        // OnContactRemoved：两个刚体分离，接触消失。
        // 参数是子形状 ID 对（SubShapeIDPair），因为一个复杂刚体可以有多个子形状。
        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override;
    };

} // DE

#endif //DEARENGINE_DECONTACTLISTENER_H
