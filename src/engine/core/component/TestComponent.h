//
// Created by ImSetal on 2026/2/12.
//

#ifndef DEARENGINE_TESTCOMPONENT_H
#define DEARENGINE_TESTCOMPONENT_H
#include "IComponent.h"

namespace DE {
    class TestComponent : public DE::IComponent {
    public:
        TestComponent() = default;
        ~TestComponent() = default;

        void Start() override;
        void Iterate() override;
    };
}

#endif //DEARENGINE_TESTCOMPONENT_H