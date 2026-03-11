/**
 * @file SceneSerialization.cpp
 * @brief 注册可序列化组件的工厂
 */
#include "SceneSerialization.h"
#include "../core/component/TransformComponent.h"
#include "../core/component/CameraComponent.h"
#include "../core/component/TestCubeComponent.h"

namespace DE {

void RegisterSceneComponentFactories() {
    RegisterComponentFactory("TransformComponent", typeid(TransformComponent),
                            [] { return std::make_unique<TransformComponent>(); });
    RegisterComponentFactory("CameraComponent", typeid(CameraComponent),
                            [] { return std::make_unique<CameraComponent>(); });
    RegisterComponentFactory("TestCubeComponent", typeid(TestCubeComponent),
                            [] { return std::make_unique<TestCubeComponent>(); });
}

} // namespace DE
