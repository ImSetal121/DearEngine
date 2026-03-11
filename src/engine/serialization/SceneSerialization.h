/**
 * @file SceneSerialization.h
 * @brief 场景中组件的工厂注册，供 Save/Load 时按类型名创建组件
 */
#pragma once

#include "../core/component/IComponent.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace DE {

/// 组件类型名 -> 工厂：创建该类型组件的 unique_ptr
inline std::unordered_map<std::string, std::function<std::unique_ptr<IComponent>()>> g_componentFactories;
/// 组件类型名 -> type_index（用于反序列化后放入 Entity::components）
inline std::unordered_map<std::string, std::type_index> g_componentTypeIndex;

inline void RegisterComponentFactory(const std::string &typeName,
                                     std::type_index typeId,
                                     std::function<std::unique_ptr<IComponent>()> factory) {
    g_componentFactories[typeName] = std::move(factory);
    g_componentTypeIndex.insert_or_assign(typeName, typeId);
}

inline std::optional<std::reference_wrapper<std::function<std::unique_ptr<IComponent>()>>>
GetComponentFactory(const std::string &typeName) {
    auto it = g_componentFactories.find(typeName);
    if (it == g_componentFactories.end())
        return std::nullopt;
    return std::ref(it->second);
}

inline std::type_index GetComponentTypeIndex(const std::string &typeName) {
    auto it = g_componentTypeIndex.find(typeName);
    if (it == g_componentTypeIndex.end())
        return std::type_index(typeid(void));
    return it->second;
}

/**
 * 新增场景组件时，在 ReflectInit.h 的 Init() 中加一行：
 *   DE_REGISTER_SCENE_COMPONENT(YourComponent, "YourComponent");
 * 并加上对应 #include，即可同时完成反射与场景序列化工厂注册。
 */
#define DE_REGISTER_SCENE_COMPONENT(Type, Name) do { \
    Type::MakeReflectable(); \
    RegisterComponentFactory(Name, typeid(Type), [] { return std::make_unique<Type>(); }); \
} while(0)

} // namespace DE
