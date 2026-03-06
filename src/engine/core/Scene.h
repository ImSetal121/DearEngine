//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ISCENE_H
#define DEARENGINE_ISCENE_H

#include <vector>

#include "Entity.h"
#include "component/CameraComponent.h"

namespace DE {
    class Scene {
    public:
        std::string name;
        CameraComponent* main_camera;

        std::vector<std::unique_ptr<Entity>> root;

        ~Scene() = default;
    };
}

#endif //DEARENGINE_ISCENE_H