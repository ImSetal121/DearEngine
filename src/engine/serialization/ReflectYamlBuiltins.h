/**
 * @file ReflectYamlBuiltins.h
 * @brief 内置类型（glm::vec3、TransformSpace 等）的 YAML 序列化/反序列化注册
 */
#pragma once

#include "ReflectYaml.h"

#include "../core/component/TransformSpace.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace DE {
namespace Reflect {

inline void InitReflectYamlSerializers() {
    RegisterTypeYamlSerializer(typeid(std::string), [](const std::any &a) {
        return YAML::Node(std::any_cast<const std::string &>(a));
    });
    RegisterTypeYamlDeserializer(typeid(std::string), [](const YAML::Node &node) -> std::any {
        if (!node.IsScalar())
            return std::any{};
        return std::any(node.as<std::string>(""));
    });

    RegisterTypeYamlSerializer(typeid(float), [](const std::any &a) {
        return YAML::Node(std::any_cast<float>(a));
    });
    RegisterTypeYamlDeserializer(typeid(float), [](const YAML::Node &node) -> std::any {
        if (!node.IsScalar())
            return std::any{};
        return std::any(node.as<float>(0.f));
    });

    RegisterTypeYamlSerializer(typeid(glm::vec3), [](const std::any &a) {
        const auto &v = std::any_cast<glm::vec3>(a);
        YAML::Node n(YAML::NodeType::Sequence);
        n.push_back(v.x);
        n.push_back(v.y);
        n.push_back(v.z);
        n.SetStyle(YAML::EmitterStyle::Flow);  // 单行输出 [x, y, z]
        return n;
    });
    RegisterTypeYamlDeserializer(typeid(glm::vec3), [](const YAML::Node &node) -> std::any {
        if (!node.IsSequence() || node.size() < 3)
            return std::any{};
        return std::any(glm::vec3(
            node[0].as<float>(0.f),
            node[1].as<float>(0.f),
            node[2].as<float>(0.f)));
    });

    RegisterTypeYamlSerializer(typeid(TransformSpace), [](const std::any &a) {
        const char *names[] = {"ParentSpace", "WorldSpace"};
        int idx = static_cast<int>(std::any_cast<TransformSpace>(a));
        if (idx < 0 || idx >= 2)
            idx = 0;
        return YAML::Node(names[idx]);
    });
    RegisterTypeYamlDeserializer(typeid(TransformSpace), [](const YAML::Node &node) -> std::any {
        if (!node.IsScalar())
            return std::any(TransformSpace::ParentSpace);
        std::string s = node.as<std::string>();
        if (s == "WorldSpace")
            return std::any(TransformSpace::WorldSpace);
        return std::any(TransformSpace::ParentSpace);
    });

    RegisterTypeYamlSerializer(typeid(glm::vec4), [](const std::any &a) {
        const auto &v = std::any_cast<glm::vec4>(a);
        YAML::Node n(YAML::NodeType::Sequence);
        n.push_back(v.x);
        n.push_back(v.y);
        n.push_back(v.z);
        n.push_back(v.w);
        n.SetStyle(YAML::EmitterStyle::Flow);
        return n;
    });
    RegisterTypeYamlDeserializer(typeid(glm::vec4), [](const YAML::Node &node) -> std::any {
        if (!node.IsSequence() || node.size() < 4)
            return std::any{};
        return std::any(glm::vec4(
            node[0].as<float>(0.f),
            node[1].as<float>(0.f),
            node[2].as<float>(0.f),
            node[3].as<float>(1.f)));
    });
}

} // namespace Reflect
} // namespace DE
