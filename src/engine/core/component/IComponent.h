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

        virtual bool Start(void *appstate) {return true;};
        virtual bool Iterate(void *appstate) {return true;};

        virtual ~IComponent() = default;
    };

}

#endif //DEARENGINE_ICOMPONENT_H