//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ICOMPONENT_H
#define DEARENGINE_ICOMPONENT_H

#include <typeinfo>

#include "../DObject.h"
#include "../render_context/RenderContext.h"

namespace DE {

    class Entity;  // 前向声明，避免与 Entity.h 循环包含

    class IComponent : public DObject {
    public:
        virtual const char* GetComponentName() const {
            return typeid(*this).name();
        };

        virtual bool EditorStart(void *appstate) {return true;};
        virtual bool Start(void *appstate) {return true;};
        virtual bool Event() {return true;};
        virtual bool EditorIterate(void *appstate) {return true;};
        virtual bool LogicIterate(void *appstate) {return true;};
        virtual bool RenderIterate(void *appstate, RenderContext* render_context) {return true;};
        virtual bool End() {return true;};
        virtual bool Quit() {return true;};

        /** 所属实体，未挂到实体时为 nullptr。获取同实体其他组件请用 GetOwner()->GetComponent<T>()，调用方需包含 Entity.h。 */
        Entity* GetOwner() const { return owner_; }
        void SetOwner(Entity* e) { owner_ = e; }

        virtual ~IComponent() = default;
    private:
        Entity* owner_ = nullptr;

        friend class Entity;
    };
}

#endif //DEARENGINE_ICOMPONENT_H