//
// Created by ImSetal on 2026/2/12.
//

#ifndef DEARENGINE_TESTCOMPONENT_H
#define DEARENGINE_TESTCOMPONENT_H
#include "IComponent.h"

namespace DE {
    class TestComponent : public IComponent {
    public:
        int time = 0;

        const char* GetComponentName() const override;

        bool Start(void *appstate) override;
        bool Iterate(void *appstate) override;

        TestComponent() = default;
        ~TestComponent() = default;
    };
}

#endif //DEARENGINE_TESTCOMPONENT_H