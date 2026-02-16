//
// Created by ImSetal on 2026/2/16.
//

#ifndef DEARENGINE_RENDERCONTEXT_H
#define DEARENGINE_RENDERCONTEXT_H
#include "ICamera.h"

namespace DE {
    struct RenderContext {
        ICamera* camera = nullptr;
    };
}

#endif //DEARENGINE_RENDERCONTEXT_H