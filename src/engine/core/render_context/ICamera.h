//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_CAMERACOMPONENT_H
#define DEARENGINE_CAMERACOMPONENT_H
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace DE {
    class ICamera {
    public:
        // ========== Transform 引用（必须绑定）==========
        glm::vec3* position = nullptr;   // 绑定外部 position
        glm::vec3* rotation = nullptr;   // 绑定外部 rotation (pitch, yaw, roll)

        // ========== 相机参数 ==========
        float Fov = 60.0f;
        float NearPlane = 0.1f;
        float FarPlane = 1000.0f;

        // ========== 方向向量计算 ==========
        // rotation 约定: x=pitch, y=yaw, z=roll
        glm::vec3 GetFront() const {
            float pitch = rotation->x;
            float yaw = rotation->y;
            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            return glm::normalize(front);
        }

        glm::vec3 GetRight() const {
            return glm::normalize(glm::cross(GetFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
        }

        glm::vec3 GetUp() const {
            return glm::normalize(glm::cross(GetRight(), GetFront()));
        }

        // ========== 视图矩阵 ==========
        glm::mat4 GetViewMatrix() const {
            return glm::lookAt(*position, *position + GetFront(), GetUp());
        }

        ICamera() = default;
        ICamera(glm::vec3* pos, glm::vec3* rot) : position(pos), rotation(rot) {}
    };
} // DE

#endif //DEARENGINE_CAMERACOMPONENT_H