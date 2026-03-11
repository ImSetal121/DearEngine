/**
 * @file ReflectYaml.cpp
 * @brief 反射 YAML 序列化/反序列化：按类型注册，自动遍历反射成员
 */
#include "ReflectYaml.h"

#include <fstream>

#include "../core/component/TransformSpace.h"
#include "../reflection/Registry.h"

#include <glm/vec3.hpp>

namespace DE {
namespace Reflect {

namespace {

std::unordered_map<std::type_index, TypeYamlSerializer> g_typeYamlSerializers;
std::unordered_map<std::type_index, TypeYamlDeserializer> g_typeYamlDeserializers;

} // namespace

void RegisterTypeYamlSerializer(std::type_index typeId, TypeYamlSerializer serializer) {
    g_typeYamlSerializers[typeId] = std::move(serializer);
}

void RegisterTypeYamlDeserializer(std::type_index typeId, TypeYamlDeserializer deserializer) {
    g_typeYamlDeserializers[typeId] = std::move(deserializer);
}

YAML::Node SerializeReflectedToYaml(const void *obj, const std::string &typeName) {
    TypeDescriptor &desc = GetByName(typeName);
    YAML::Node root(YAML::NodeType::Map);
    std::any objAny = desc.WrapObject(obj);
    if (!objAny.has_value())
        return root;

    for (const auto &mv : desc.member_vars()) {
        std::any val = mv.GetValueAny(objAny);
        if (val.has_value()) {
            auto it = g_typeYamlSerializers.find(mv.type());
            if (it != g_typeYamlSerializers.end())
                root[mv.name()] = it->second(val);
        }
    }
    return root;
}

bool DeserializeReflectedFromYaml(void *obj, const std::string &typeName, const YAML::Node &root) {
    if (!root.IsMap())
        return false;
    TypeDescriptor &desc = GetByName(typeName);
    std::any objAny = desc.WrapMutableObject(obj);
    if (!objAny.has_value())
        return false;

    for (const auto &mv : desc.member_vars()) {
        YAML::Node node = root[mv.name()];
        if (!node.IsDefined())
            continue;
        auto it = g_typeYamlDeserializers.find(mv.type());
        if (it == g_typeYamlDeserializers.end())
            continue;
        try {
            std::any val = it->second(node);
            if (val.has_value())
                mv.SetValueAny(objAny, std::move(val));
        } catch (...) {
            return false;
        }
    }
    return true;
}

bool SaveReflectedToYamlFile(const void *obj,
                             const std::string &typeName,
                             const std::string &filePath) {
    YAML::Node root = SerializeReflectedToYaml(obj, typeName);
    if (!root.IsMap())
        return false;
    try {
        std::ofstream f(filePath);
        if (!f)
            return false;
        f << root;
        return true;
    } catch (...) {
        return false;
    }
}

bool LoadReflectedFromYamlFile(void *obj,
                               const std::string &typeName,
                               const std::string &filePath) {
    try {
        YAML::Node root = YAML::LoadFile(filePath);
        return DeserializeReflectedFromYaml(obj, typeName, root);
    } catch (...) {
        return false;
    }
}

void InitReflectYamlSerializers() {
    RegisterTypeYamlSerializer(typeid(glm::vec3), [](const std::any &a) {
        const auto &v = std::any_cast<glm::vec3>(a);
        YAML::Node n(YAML::NodeType::Map);
        n["x"] = v.x;
        n["y"] = v.y;
        n["z"] = v.z;
        return n;
    });
    RegisterTypeYamlDeserializer(typeid(glm::vec3), [](const YAML::Node &node) -> std::any {
        if (!node.IsMap())
            return std::any{};
        return std::any(glm::vec3(
            node["x"].as<float>(0.f),
            node["y"].as<float>(0.f),
            node["z"].as<float>(0.f)));
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
}

} // namespace Reflect
} // namespace DE
