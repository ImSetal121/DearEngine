//
// Created by ImSetal on 2026/2/11.
//

#include "Entity.h"

#include <algorithm>
#include <utility>

namespace DE {

    Entity* Entity::AddChild(std::unique_ptr<Entity> child) {
        child->parent_ = this;
        Entity* raw = child.get();
        children_.push_back(std::move(child));
        return raw;
    }

    std::unique_ptr<Entity> Entity::DetachChild(Entity* child) {
        auto it = std::find_if(children_.begin(), children_.end(),
            [child](const auto& p) { return p.get() == child; });
        if (it == children_.end()) return nullptr;
        std::unique_ptr<Entity> result = std::move(*it);
        children_.erase(it);
        result->parent_ = nullptr;
        return result;
    }

    const std::vector<std::unique_ptr<Entity>> &Entity::GetChildren() const {
        return children_;
    }

    const std::unordered_map<std::type_index, std::unique_ptr<IComponent>> &Entity::GetComponentList() const {
        return components_;
    }

}
