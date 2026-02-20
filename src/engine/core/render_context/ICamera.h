//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_CAMERACOMPONENT_H
#define DEARENGINE_CAMERACOMPONENT_H
#include <glm/glm.hpp>

namespace DE {
    class ICamera {
    public:
        //默认参数
        const float YAW = -90.0f;
        const float PITCH = 0.0f;
        const float SPEED = 2.5f;
        const float FOV = 60.0f;

        //摄像机属性
        glm::vec3 Position;//相机位置
        glm::vec3 Front;//相机前方向
        glm::vec3 Up;//相机上方向
        glm::vec3 Right;//相机右方向
        glm::vec3 WorldUp;//世界坐标系的上方向
        //欧拉角
        float Yaw;//偏航角
        float Pitch;//俯仰角
        //摄像机设置
        float Fov;//视野

        /** 返回视图矩阵 */
        glm::mat4 GetViewMatrix();
        /** 根据 Yaw/Pitch 更新 Front/Right/Up 向量 */
        void UpdateVectors();

        ICamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);
    };
} // DE

#endif //DEARENGINE_CAMERACOMPONENT_H