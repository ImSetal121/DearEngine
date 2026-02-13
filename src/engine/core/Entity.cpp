//
// Created by ImSetal on 2026/2/11.
//

#include "Entity.h"

#include <algorithm>
#include <utility>

namespace DE {

    Entity* Entity::AddChild(std::unique_ptr<Entity> child) {
        child->parent = this;
        Entity* raw = child.get();
        children.push_back(std::move(child));
        return raw;
    }

    std::unique_ptr<Entity> Entity::DetachChild(Entity* child) {
        auto it = std::find_if(children.begin(), children.end(),
            [child](const auto& p) { return p.get() == child; });
        if (it == children.end()) return nullptr;
        std::unique_ptr<Entity> result = std::move(*it);
        children.erase(it);
        result->parent = nullptr;
        return result;
    }

}
