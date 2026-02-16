//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_TESTCUBECOMPONENT_H
#define DEARENGINE_TESTCUBECOMPONENT_H
#include "IComponent.h"

namespace DE {
    class TestCubeComponent : public IComponent {
    public:
        //立方体顶点数据
        float cubeVertices[48] = {
            // positions
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f
        };

        const char* GetComponentName() const override;

        bool Init(void *appstate) override;
        bool Event() override;
        bool LogicIterate(void *appstate) override;
        bool RenderIterate(void *appstate, RenderContext* render_context) override;
        bool Quit() override;
    private:
        unsigned int VBO;
        unsigned int cubeVAO;
    };
} // DE

#endif //DEARENGINE_TESTCUBECOMPONENT_H