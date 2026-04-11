//
// Created by ImSetal on 2026/2/11.
//

#include "Entity.h"

#include <algorithm>
#include <utility>

namespace DE {

    Entity* Entity::GetChildByName(const std::string& name) {
        for (auto& child : children) {
            if (child->name == name) return child.get();
            if (auto* found = child->GetChildByName(name)) return found;
        }
        return nullptr;
    }

    Entity* Entity::AddChild(std::unique_ptr<Entity> child) {
        if (child->name.empty()) {
            int i = 1;
            while (true) {
                if (GetChildByName("entity " + std::to_string(i)) == nullptr) {
                    child->name = "entity " + std::to_string(i);
                    break;
                }
                i++;
            }
        }else {
            if (GetChildByName(child->name) != nullptr) {
                int i = 1;
                std::string original_name = child->name;
                while (true) {
                    if (GetChildByName(original_name + " " + std::to_string(i)) == nullptr) {
                        child->name = original_name + " " + std::to_string(i);
                        break;
                    }
                    i++;
                }
            }
        }
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
