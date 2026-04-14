//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ISCENE_H
#define DEARENGINE_ISCENE_H

#include <vector>

#include "DObject.h"
#include "Entity.h"
#include "component/CameraComponent.h"
#include "./reflection/Reflect.h"
#include "./reflection/Registry.h"

namespace DE {
    class Scene : public DObject {
    public:
        std::string name;
        std::string save_path;  // 不参与反射
        CameraComponent* main_camera;
        std::vector<std::unique_ptr<Entity>> root;

        void AddEntity(std::unique_ptr<Entity> entity);
        Entity* GetEntityByName(const std::string& name);
        std::unique_ptr<Entity> CloneEntity(Entity* e);
        void Save();
        void Load();
        void Init();
        void End();
        void EditorStartAllEntity(void* appstate);
        void EditorIterateAllEntity(void* appstate);

        ~Scene() = default;
        static void MakeReflectable() {
            DE::Reflect::AddClass<Scene>("Scene")
                .AddMemberVar("name", &Scene::name);
        }
    };
}

#endif //DEARENGINE_ISCENE_H