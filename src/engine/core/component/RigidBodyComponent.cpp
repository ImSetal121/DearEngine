//
// Created by ImSetal on 2026/4/13.
//

#include "RigidBodyComponent.h"
#include "TransformComponent.h"
#include "../Log.h"
#include "../Entity.h"
#include "../physics/Physics.h"
#include <glm/gtc/quaternion.hpp>
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"

namespace DE {
    const char * RigidBodyComponent::GetComponentName() const {
        return "刚体组件";
    }

    bool RigidBodyComponent::EditorStart(void *appstate) {
        return IComponent::EditorStart(appstate);
    }

    bool RigidBodyComponent::Start(void *appstate) {
        auto transform = GetOwner()->GetComponent<TransformComponent>();
        if (transform == nullptr)
            DE::Log::Warning("["+GetOwner()->name+"]["+ RigidBodyComponent::GetComponentName()+"]需要Transform组件才能够正常工作.");
        else {
            transform->SyncWorldTransform();

            // rotation_world 已是四元数，直接使用
            const glm::quat& q = transform->rotation_world;
            JPH::Quat jph_rot(q.x, q.y, q.z, q.w);
            JPH::BodyCreationSettings shape_settings;
            switch (shape_type) {
                case ShapeType::Box: {
                    shape_settings = JPH::BodyCreationSettings(
                        new JPH::BoxShape(JPH::RVec3(box_half_extent.x, box_half_extent.y, box_half_extent.z)),
                        JPH::RVec3(transform->position_world.x, transform->position_world.y, transform->position_world.z),
                        jph_rot,
                        motion_type,
                        motion_type == JPH::EMotionType::Static ? Layers::NON_MOVING : Layers::MOVING );
                    break;
                }
                default: {
                    Log::Warning("["+GetOwner()->name+"]["+ RigidBodyComponent::GetComponentName()+"]形状类型信息缺失.");
                }
            }

            body_id_ = Physics::Instance().GetPhysicsSystem()->GetBodyInterface().CreateAndAddBody(shape_settings, JPH::EActivation::Activate);
            Physics::Instance().RegisterBody(body_id_, this);
        }
        return IComponent::Start(appstate);
    }

    bool RigidBodyComponent::Event() {
        return IComponent::Event();
    }

    bool RigidBodyComponent::EditorIterate(void *appstate) {
        return IComponent::EditorIterate(appstate);
    }

    bool RigidBodyComponent::LogicIterate(void *appstate) {
        auto transform = GetOwner()->GetComponent<TransformComponent>();
        if (transform == nullptr)
            DE::Log::Warning("["+GetOwner()->name+"]["+ RigidBodyComponent::GetComponentName()+"]需要Transform组件才能够正常工作.");
        else {
            if (!Physics::Instance().GetPhysicsSystem())
                return IComponent::LogicIterate(appstate);
            auto& bi = Physics::Instance().GetPhysicsSystem()->GetBodyInterface();
            JPH::RVec3 pos = bi.GetPosition(body_id_);
            JPH::Quat rot = bi.GetRotation(body_id_);
            glm::quat q(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
            // 直接写四元数，无欧拉角中转
            transform->rotation_world = q;
            // 同步 euler 仅供编辑器显示（近似值，不参与物理计算）
            transform->rotation = glm::degrees(glm::eulerAngles(q));
            transform->SetWorldPosition(glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ()));
        }
        return IComponent::LogicIterate(appstate);
    }

    bool RigidBodyComponent::RenderIterate(void *appstate, RenderContext *render_context) {
        return IComponent::RenderIterate(appstate, render_context);
    }

    bool RigidBodyComponent::End() {
        return IComponent::End();
    }

    bool RigidBodyComponent::Quit() {
        return IComponent::Quit();
    }
} // DE