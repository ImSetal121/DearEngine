//
// Created by ImSetal on 2026/4/9.
//

#ifndef DEARENGINE_DIRLIGHTCOMPONENT_H
#define DEARENGINE_DIRLIGHTCOMPONENT_H
#include "IComponent.h"
#include "../render_context/IDirLight.h"

namespace DE {

class DirLightComponent : public IComponent {
    public:
    const char* GetComponentName() const override;
    IDirLight* dir_light = nullptr;

    bool EditorStart(void *appstate) override;
    bool Start(void *appstate) override;
    bool RenderIterate(void *appstate, RenderContext* render_context) override;

    DirLightComponent() = default;
    static void MakeReflectable() {
        DE::Reflect::AddClass<DirLightComponent>("DirLightComponent")
            .AddMemberVar("dir_light", &DirLightComponent::dir_light);
    }

};

} // DE

#endif //DEARENGINE_DIRLIGHTCOMPONENT_H
