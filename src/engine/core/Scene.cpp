//
// Scene Save/Load：序列化/反序列化场景中所有实体与组件
//
#include "Scene.h"
#include "Entity.h"
#include "component/IComponent.h"
#include "component/CameraComponent.h"
#include "./reflection/Reflect.h"
#include "./reflection/Registry.h"
#include "./serialization/ReflectYaml.h"
#include "./serialization/SceneSerialization.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>

namespace DE {

namespace {

// 在实体树中查找拥有指定 CameraComponent 的实体，返回其 name；未找到或 cam 为 null 返回空串
static std::string FindEntityNameWithCamera(Entity* e, CameraComponent* cam) {
    if (!e || !cam) return {};
    if (e->GetComponent<CameraComponent>() == cam)
        return e->name;
    for (const auto& child : e->children) {
        std::string name = FindEntityNameWithCamera(child.get(), cam);
        if (!name.empty()) return name;
    }
    return {};
}

// 在实体树中按 name 查找实体
static Entity* FindEntityByName(Entity* e, const std::string& name) {
    if (!e || name.empty()) return nullptr;
    if (e->name == name) return e;
    for (const auto& child : e->children) {
        Entity* found = FindEntityByName(child.get(), name);
        if (found) return found;
    }
    return nullptr;
}

// 在场景的 root 及子树中按 name 查找实体
static Entity* FindEntityByNameInScene(Scene* scene, const std::string& name) {
    if (!scene || name.empty()) return nullptr;
    for (const auto& root : scene->root) {
        Entity* found = FindEntityByName(root.get(), name);
        if (found) return found;
    }
    return nullptr;
}

// 将单个实体（含子实体与组件）序列化为 YAML 节点
YAML::Node SerializeEntity(const Entity *e) {
    YAML::Node entityNode(YAML::NodeType::Map);
    YAML::Node entityData = Reflect::SerializeReflectedToYaml(e, "Entity");
    if (entityData["Entity"].IsDefined())
        entityNode["Entity"] = entityData["Entity"];

    YAML::Node componentsNode(YAML::NodeType::Map);
    for (const auto &kv : e->components) {
        const std::string *typeName = nullptr;
        {
            auto nameOpt = Reflect::GetTypeName(kv.first);
            if (!nameOpt.has_value())
                continue;
            typeName = &*nameOpt;
        }
        YAML::Node compRoot = Reflect::SerializeReflectedToYaml(kv.second.get(), *typeName);
        if (compRoot[*typeName].IsDefined())
            componentsNode[*typeName] = compRoot[*typeName];
    }
    entityNode["components"] = componentsNode;

    YAML::Node childrenNode(YAML::NodeType::Sequence);
    for (const auto &child : e->children)
        childrenNode.push_back(SerializeEntity(child.get()));
    entityNode["children"] = childrenNode;

    return entityNode;
}

// 从 YAML 节点反序列化出实体（含组件与子实体），返回 unique_ptr<Entity>
std::unique_ptr<Entity> DeserializeEntity(const YAML::Node &node) {
    if (!node.IsMap())
        return nullptr;

    auto entity = std::make_unique<Entity>();
    YAML::Node entityData = node["Entity"];
    if (entityData.IsDefined() && entityData.IsMap()) {
        YAML::Node wrap(YAML::NodeType::Map);
        wrap["Entity"] = entityData;
        Reflect::DeserializeReflectedFromYaml(entity.get(), "Entity", wrap);
    }

    YAML::Node componentsNode = node["components"];
    if (componentsNode.IsMap()) {
        for (auto it = componentsNode.begin(); it != componentsNode.end(); ++it) {
            std::string typeName = it->first.as<std::string>("");
            const YAML::Node &compNode = it->second;
            if (!compNode.IsDefined() || typeName.empty())
                continue;

            auto factoryOpt = GetComponentFactory(typeName);
            if (!factoryOpt.has_value())
                continue;

            std::unique_ptr<IComponent> comp = factoryOpt->get()();
            if (!comp)
                continue;

            YAML::Node wrap(YAML::NodeType::Map);
            wrap[typeName] = compNode;
            if (Reflect::DeserializeReflectedFromYaml(comp.get(), typeName, wrap)) {
                comp->SetOwner(entity.get());
                std::type_index ti = GetComponentTypeIndex(typeName);
                if (ti != std::type_index(typeid(void)))
                    entity->components[ti] = std::move(comp);
            }
        }
    }

    YAML::Node childrenNode = node["children"];
    if (childrenNode.IsSequence()) {
        for (size_t i = 0; i < childrenNode.size(); ++i) {
            std::unique_ptr<Entity> child = DeserializeEntity(childrenNode[i]);
            if (child) {
                child->parent = entity.get();
                entity->children.push_back(std::move(child));
            }
        }
    }

    return entity;
}

} // namespace

Entity* Scene::GetEntityByName(const std::string& name) {
    return FindEntityByNameInScene(this, name);
}

void Scene::AddEntity(std::unique_ptr<Entity> entity) {
    if (entity->name.empty()) {
        int i = 1;
        while (true) {
            if (GetEntityByName("entity " + std::to_string(i)) == nullptr) {
                entity->name = "entity " + std::to_string(i);
                break;
            }
            i++;
        }
    }else {
        if (GetEntityByName(entity->name) != nullptr) {
            int i = 1;
            std::string original_name = entity->name;
            while (true) {
                if (GetEntityByName(original_name + " " + std::to_string(i)) == nullptr) {
                    entity->name = original_name + " " + std::to_string(i);
                    break;
                }
                i++;
            }
        }
    }
    root.push_back(std::move(entity));
}

void Scene::Save() {
    if (save_path.empty())
        return;

    YAML::Node doc(YAML::NodeType::Map);
    YAML::Node sceneData = Reflect::SerializeReflectedToYaml(this, "Scene");
    if (sceneData["Scene"].IsDefined())
        doc["Scene"] = sceneData["Scene"];

    // 主相机：记录拥有 main_camera 的实体名称
    std::string mainCameraEntityName;
    for (const auto& e : this->root) {
        mainCameraEntityName = FindEntityNameWithCamera(e.get(), main_camera);
        if (!mainCameraEntityName.empty()) break;
    }
    if (!mainCameraEntityName.empty())
        doc["Scene"]["main_camera_entity_name"] = mainCameraEntityName;

    YAML::Node rootEntities(YAML::NodeType::Sequence);
    for (const auto &e : this->root)
        rootEntities.push_back(SerializeEntity(e.get()));
    doc["root"] = rootEntities;

    try {
        std::ofstream f(save_path);
        if (!f)
            return;
        f << doc;
    } catch (...) {
    }
}

void Scene::Load() {
    if (save_path.empty())
        return;

    YAML::Node doc;
    try {
        doc = YAML::LoadFile(save_path);
    } catch (...) {
        return;
    }
    if (!doc.IsMap())
        return;

    YAML::Node sceneData = doc["Scene"];
    if (sceneData.IsDefined() && sceneData.IsMap()) {
        YAML::Node wrap(YAML::NodeType::Map);
        wrap["Scene"] = sceneData;
        Reflect::DeserializeReflectedFromYaml(this, "Scene", wrap);
    }

    YAML::Node rootEntitiesNode = doc["root"];
    if (!rootEntitiesNode.IsSequence())
        return;

    this->root.clear();
    for (size_t i = 0; i < rootEntitiesNode.size(); ++i) {
        std::unique_ptr<Entity> e = DeserializeEntity(rootEntitiesNode[i]);
        if (e)
            this->root.push_back(std::move(e));
    }

    // 恢复主相机：按保存的实体名查找并设置 main_camera
    main_camera = nullptr;
    YAML::Node sceneNode = doc["Scene"];
    if (sceneNode.IsMap()) {
        YAML::Node nameNode = sceneNode["main_camera_entity_name"];
        if (nameNode.IsDefined() && nameNode.IsScalar()) {
            std::string name = nameNode.as<std::string>("");
            Entity* entity = FindEntityByNameInScene(this, name);
            if (entity)
                main_camera = entity->GetComponent<CameraComponent>();
        }
    }

    // 读取后将场景名设为文件名（除后缀）
    name = std::filesystem::path(save_path).stem().string();
}

} // namespace DE
