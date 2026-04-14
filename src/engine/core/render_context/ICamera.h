//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_CAMERACOMPONENT_H
#define DEARENGINE_CAMERACOMPONENT_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../DObject.h"
#include "../reflection/Reflect.h"
#include "../reflection/Registry.h"

namespace DE {
    class ICamera : public DObject {
    public:
        // ========== Transform 引用（必须绑定）==========
        glm::vec3* position = nullptr;
        glm::quat* rotation = nullptr;   // 绑定外部 rotation_world（四元数）

        // ========== 相机参数 ==========
        float fov = 60.0f;
        float near_lane = 0.1f;
        float far_plane = 1000.0f;
        glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        static void MakeReflectable() {
            DE::Reflect::AddClass<ICamera>("ICamera")
                .AddMemberVar("fov", &ICamera::fov)
                .AddMemberVar("near_lane", &ICamera::near_lane)
                .AddMemberVar("far_plane", &ICamera::far_plane)
                .AddMemberVar("clear_color", &ICamera::clear_color);
        }

        // ========== 方向向量计算 ==========
        // 默认朝向 -Z，rotation 四元数直接旋转基向量
        glm::vec3 GetFront() const {
            return *rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        }

        glm::vec3 GetRight() const {
            return *rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        }

        glm::vec3 GetUp() const {
            return *rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // ========== 视图矩阵 ==========
        glm::mat4 GetViewMatrix() const {
            return glm::lookAt(*position, *position + GetFront(), GetUp());
        }

        ICamera(glm::vec3* pos, glm::quat* rot) : position(pos), rotation(rot) {}
    };
} // DE

#endif //DEARENGINE_CAMERACOMPONENT_H