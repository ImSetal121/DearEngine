/**
 * @file ReflectYaml.h
 * @brief 基于反射系统将对象反射字段自动序列化/反序列化为 YAML（按类型注册，无需逐成员手写）
 */
#pragma once

#include "../reflection/Reflect.h"
#include "../reflection/TypeDescriptor.h"

#include "yaml-cpp/yaml.h"

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

/// 为指定类型注册 YAML 序列化器
void RegisterTypeYamlSerializer(std::type_index typeId, TypeYamlSerializer serializer);

/// 为指定类型注册 YAML 反序列化器
void RegisterTypeYamlDeserializer(std::type_index typeId, TypeYamlDeserializer deserializer);

/// 将已注册反射的对象按其成员类型自动序列化为 YAML 节点
YAML::Node SerializeReflectedToYaml(const void *obj, const std::string &typeName);

/// 从 YAML 节点按成员名 + 类型反序列化到已注册反射的对象（obj 为非 const 指针）
bool DeserializeReflectedFromYaml(void *obj, const std::string &typeName, const YAML::Node &root);

/// 将反射对象序列化并写入文件
bool SaveReflectedToYamlFile(const void *obj,
                             const std::string &typeName,
                             const std::string &filePath);

/// 从文件读取 YAML 并反序列化到对象
bool LoadReflectedFromYamlFile(void *obj,
                               const std::string &typeName,
                               const std::string &filePath);

/// 注册内置类型（glm::vec3、TransformSpace 等）的 YAML 序列化/反序列化器，在反射 Init 之后调用
void InitReflectYamlSerializers();

} // namespace Reflect
} // namespace DE
