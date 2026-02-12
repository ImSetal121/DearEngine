//
// Created by ImSetal on 2026/2/11.
//

#ifndef DEARENGINE_ICOMPONENT_H
#define DEARENGINE_ICOMPONENT_H

namespace DE {

    class IComponent {
        public:
        virtual ~IComponent() = default;

        virtual void Start() = 0;
        virtual void Iterate() = 0;
    };

}

#endif //DEARENGINE_ICOMPONENT_H