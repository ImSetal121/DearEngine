//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ICOMPONENT_H
#define DEARENGINE_ICOMPONENT_H

#include <typeinfo>

namespace DE {

    class IComponent {
        public:
        virtual const char* GetComponentName() const {
            return typeid(*this).name();
        };

        virtual void Start(void *appstate) = 0;
        virtual void Iterate(void *appstate) = 0;

        virtual ~IComponent() = default;
    };

}

#endif //DEARENGINE_ICOMPONENT_H