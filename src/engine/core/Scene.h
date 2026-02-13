//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ISCENE_H
#define DEARENGINE_ISCENE_H

#include <vector>

#include "Entity.h"

namespace DE {
    class Scene {
    public:
        std::string name;

        std::vector<std::unique_ptr<Entity>> GetRootList;

        ~Scene() = default;
    public:
        std::vector<std::unique_ptr<Entity>> root;
    };
}

#endif //DEARENGINE_ISCENE_H