//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_RENDERCONTEXT_H
#define DEARENGINE_RENDERCONTEXT_H
#include "ICamera.h"
#include "IDirLight.h"

namespace DE {
    struct RenderContext {
        unsigned int* program = nullptr;

        int* screenWidth = nullptr;
        int* screenHeight = nullptr;

        ICamera* camera = nullptr;
        IDirLight* dirLight = nullptr;
    };
}

#endif //DEARENGINE_RENDERCONTEXT_H