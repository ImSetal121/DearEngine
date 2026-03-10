//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_IENTITY_H
#define DEARENGINE_IENTITY_H
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "component/IComponent.h"

namespace DE {

    class Entity : public DObject {
    public:
        std::string name;
        Entity* parent = nullptr;
        std::vector<std::unique_ptr<Entity>> children;
        std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components;

        ~Entity() = default;

        // 子实体操作
        // 添加子实体
        Entity* AddChild(std::unique_ptr<Entity> child);

        // 从父节点移除（通常返回 unique_ptr 给调用方）
        std::unique_ptr<Entity> DetachChild(Entity* child);

        // 组件操作
        // 添加组件
        template<typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            auto typeId = std::type_index(typeid(T));
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = ptr.get();
            components[typeId] = std::move(ptr);
            raw->SetOwner(this);
            return raw;
        }
        
        // 获取组件（返回 nullptr 表示不存在）
        template<typename T>
        T* GetComponent() {
            auto it = components.find(std::type_index(typeid(T)));
            if (it == components.end()) return nullptr;
            return static_cast<T*>(it->second.get());
        }
        
        // 移除组件
        template<typename T>
        void RemoveComponent() {
            components.erase(std::type_index(typeid(T)));
        }
        
        // 检查是否有某类型组件
        template<typename T>
        bool HasComponent() const {
            return components.find(std::type_index(typeid(T))) != components.end();
        }
    };

}

#endif //DEARENGINE_IENTITY_H
