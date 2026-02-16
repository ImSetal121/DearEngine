//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ICOMPONENT_H
#define DEARENGINE_ICOMPONENT_H

#include <typeinfo>

#include "../render_context/RenderContext.h"

namespace DE {

    class IComponent {
        public:
        virtual const char* GetComponentName() const {
            return typeid(*this).name();
        };

        virtual bool Init(void *appstate) {return true;};
        virtual bool Event() {return true;};
        virtual bool LogicIterate(void *appstate) {return true;};
        virtual bool RenderIterate(void *appstate, RenderContext* render_context) {return true;};
        virtual bool Quit() {return true;};

        virtual ~IComponent() = default;
    };
}

#endif //DEARENGINE_ICOMPONENT_H