/**
 * @file ReflectYaml.h
 * @brief 基于反射系统将对象反射字段自动序列化/反序列化为 YAML（按类型注册，无需逐成员手写）
 */
#pragma once

#include "../reflection/Reflect.h"
#include "../reflection/TypeDescriptor.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace DE {
namespace Reflect {

/// 类型序列化器：将 std::any（该类型值）转为 YAML::Node
using TypeYamlSerializer = std::function<YAML::Node(const std::any &)>;

/// 类型反序列化器：将 YAML::Node 转为 std::any（该类型值）
using TypeYamlDeserializer = std::function<std::any(const YAML::Node &)>;

/// 全局类型序列化/反序列化器表（inline 保证全程序唯一，避免多 TU 各有一份空表）
inline std::unordered_map<std::type_index, TypeYamlSerializer> g_typeYamlSerializers;
inline std::unordered_map<std::type_index, TypeYamlDeserializer> g_typeYamlDeserializers;

inline void RegisterTypeYamlSerializer(std::type_index typeId, TypeYamlSerializer serializer) {
    g_typeYamlSerializers[typeId] = std::move(serializer);
}

inline void RegisterTypeYamlDeserializer(std::type_index typeId, TypeYamlDeserializer deserializer) {
    g_typeYamlDeserializers[typeId] = std::move(deserializer);
}

/// 序列化：类型名作为顶层键，成员作为其子节点（成员比类型多缩进一层）
inline YAML::Node SerializeReflectedToYaml(const void *obj, const std::string &typeName) {
    TypeDescriptor &desc = GetByName(typeName);
    YAML::Node members(YAML::NodeType::Map);
    std::any objAny = desc.WrapObject(obj);
    if (!objAny.has_value()) {
        YAML::Node root(YAML::NodeType::Map);
        root[typeName] = members;
        return root;
    }

    for (const auto &mv : desc.member_vars()) {
        std::any val = mv.GetValueAny(objAny);
        if (val.has_value()) {
            auto it = g_typeYamlSerializers.find(mv.type());
            if (it != g_typeYamlSerializers.end())
                members[mv.name()] = it->second(val);
        }
    }
    YAML::Node root(YAML::NodeType::Map);
    root[typeName] = members;
    return root;
}

inline bool DeserializeReflectedFromYaml(void *obj, const std::string &typeName, const YAML::Node &root) {
    if (!root.IsMap())
        return false;
    YAML::Node data = root[typeName];
    if (!data.IsDefined() || !data.IsMap())
        data = root;  // 兼容：根节点即为成员表
    TypeDescriptor &desc = GetByName(typeName);
    std::any objAny = desc.WrapMutableObject(obj);
    if (!objAny.has_value())
        return false;

    for (const auto &mv : desc.member_vars()) {
        YAML::Node node = data[mv.name()];
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

inline bool SaveReflectedToYamlFile(const void *obj,
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

inline bool LoadReflectedFromYamlFile(void *obj,
                                      const std::string &typeName,
                                      const std::string &filePath) {
    try {
        YAML::Node root = YAML::LoadFile(filePath);
        return DeserializeReflectedFromYaml(obj, typeName, root);
    } catch (...) {
        return false;
    }
}

} // namespace Reflect
} // namespace DE
