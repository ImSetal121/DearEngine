//
// Created by ImSetal on 2026/4/9.
//

#ifndef DEARENGINE_IDIRLIGHT_H
#define DEARENGINE_IDIRLIGHT_H
#include <glm/vec3.hpp>

namespace DE {
    class IDirLight {
        public:
        // 定向光方向
        glm::vec3 direction;

        // 环境光颜色
        glm::vec3 ambient = glm::vec3(0.2f);
        // 漫反射颜色
        glm::vec3 diffuse = glm::vec3(1.0f);
        // 高光颜色
        glm::vec3 specular = glm::vec3(1.0f);

        IDirLight();
        IDirLight(glm::vec3 dir) : direction(dir) {}
        IDirLight(glm::vec3 dir, glm::vec3 amb, glm::vec3 diff, glm::vec3 spec) : direction(dir), ambient(amb), diffuse(diff), specular(spec) {}
        ~IDirLight();
        static void MakeReflectable() {
            DE::Reflect::AddClass<IDirLight>("IDirLight")
                .AddMemberVar("direction", &IDirLight::direction)
                .AddMemberVar("ambient", &IDirLight::ambient)
                .AddMemberVar("diffuse", &IDirLight::diffuse)
                .AddMemberVar("specular", &IDirLight::specular);
        }
    };
}

#endif //DEARENGINE_IDIRLIGHT_H
