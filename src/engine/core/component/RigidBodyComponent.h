//
// Created by ImSetal on 2026/4/13.
//

#ifndef DEARENGINE_RIGIDBODYCOMPONENT_H
#define DEARENGINE_RIGIDBODYCOMPONENT_H
#include "IComponent.h"

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/MotionType.h"

namespace DE {

class RigidBodyComponent : public IComponent {
    public:
    // 运动类型
    JPH::EMotionType motion_type = JPH::EMotionType::Dynamic;
    // 形状
    enum class ShapeType { Box, Sphere };
    ShapeType shape_type = ShapeType::Box;
    // 形状参数
    glm::vec3 box_half_extent = glm::vec3(0.5f);
    float sphere_radius = 0.5f;
    // 回调方法
    std::function<void(const JPH::BodyID& other)> on_contact_added;
    std::function<void(const JPH::BodyID& other)> on_contact_persisted;
    std::function<void(const JPH::BodyID& other)> on_contact_removed;

    const char* GetComponentName() const override;
    bool EditorStart(void *appstate) override;
    bool Start(void *appstate) override;
    bool Event() override;
    bool EditorIterate(void *appstate) override;
    bool LogicIterate(void *appstate) override;
    bool RenderIterate(void *appstate, RenderContext* render_context) override;
    bool End() override;
    bool Quit() override;
    RigidBodyComponent() = default;
    static void MakeReflectable() {
        DE::Reflect::AddClass<RigidBodyComponent>("RigidBodyComponent")
            .AddMemberVar("motion_type", &RigidBodyComponent::motion_type)
            .AddMemberVar("shape_type", &RigidBodyComponent::shape_type)
            .AddMemberVar("box_half_extent", &RigidBodyComponent::box_half_extent)
            .AddMemberVar("sphere_radius", &RigidBodyComponent::sphere_radius);
    }
    private:
    JPH::BodyID body_id_;
};

} // DE

#endif //DEARENGINE_RIGIDBODYCOMPONENT_H
