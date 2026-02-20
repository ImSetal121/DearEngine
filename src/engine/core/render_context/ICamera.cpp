//
// Created by ImSetal on 2026/2/16.
//

#include "ICamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace DE {

    void ICamera::UpdateVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    ICamera::ICamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f))
        , Fov(FOV)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        UpdateVectors();
    }

    glm::mat4 ICamera::GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

} // DE
