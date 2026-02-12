//
// Created by ImSetal on 2026/2/12.
//

#include "TestComponent.h"

#include "../Log.h"

void DE::TestComponent::Start() {
    DE::Log::Debug("组件初始化.");
}

void DE::TestComponent::Iterate() {
    DE::Log::Debug("组件调用.");
}
