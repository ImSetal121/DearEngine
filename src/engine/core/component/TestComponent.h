//
// Created by ImSetal on 2026/2/12.
//

#ifndef DEARENGINE_TESTCOMPONENT_H
#define DEARENGINE_TESTCOMPONENT_H
#include "IComponent.h"

namespace DE {
    class TestComponent : public DE::IComponent {
    public:
        int time = 0;

        const char* GetComponentName() const override;

        void Start(void *appstate) override;
        void Iterate(void *appstate) override;

        TestComponent() = default;
        ~TestComponent() = default;
    };
}

#endif //DEARENGINE_TESTCOMPONENT_H