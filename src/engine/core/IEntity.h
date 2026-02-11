//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_IENTITY_H
#define DEARENGINE_IENTITY_H
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "IComponent.h"

namespace DE {

    class IEntity {
    public:
        virtual ~IEntity() = default;

        // 子实体操作
        // 添加子实体
        IEntity* AddChild(std::unique_ptr<IEntity> child) {
            child->parent_ = this;
            IEntity* raw = child.get();
            children_.push_back(std::move(child));
            return raw;
        }

        // 从父节点移除（通常返回 unique_ptr 给调用方）
        std::unique_ptr<IEntity> DetachChild(IEntity* child) {
            auto it = std::find_if(children_.begin(), children_.end(),
                [child](const auto& p) { return p.get() == child; });
            if (it == children_.end()) return nullptr;
            std::unique_ptr<IEntity> result = std::move(*it);
            children_.erase(it);
            result->parent_ = nullptr;
            return result;
        }

        // 递归遍历
        void ForEachEntity(std::function<void(IEntity*)> func) {
            func(this);
            for (auto& child : children_)
                child->ForEachEntity(func);
        }

        // 组件操作
        // 添加组件
        template<typename T, typename... Args>
        T* AddComponent(Args&&... args) {
            auto typeId = std::type_index(typeid(T));
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = ptr.get();
            components_[typeId] = std::move(ptr);
            return raw;
        }
        
        // 获取组件（返回 nullptr 表示不存在）
        template<typename T>
        T* GetComponent() {
            auto it = components_.find(std::type_index(typeid(T)));
            if (it == components_.end()) return nullptr;
            return static_cast<T*>(it->second.get());
        }
        
        // 移除组件
        template<typename T>
        void RemoveComponent() {
            components_.erase(std::type_index(typeid(T)));
        }
        
        // 检查是否有某类型组件
        template<typename T>
        bool HasComponent() const {
            return components_.find(std::type_index(typeid(T))) != components_.end();
        }

        // 属性Getter/Setter
        virtual std::string GetName();
        virtual void SetName(std::string name);
        std::vector<std::unique_ptr<IEntity>> GetChildren(){ return children_; }
        std::unordered_map<std::type_index, std::unique_ptr<IComponent>> GetComponentList(){ return components_; }

    private:
        IEntity* parent_ = nullptr;
        std::vector<std::unique_ptr<IEntity>> children_;
        std::unordered_map<std::type_index, std::unique_ptr<IComponent>> components_;
    };

}

#endif //DEARENGINE_IENTITY_H