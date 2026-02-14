//
// Created by ImSetal on 2026/2/12.
//

#include "TestComponent.h"

#include <format>

#include "../Log.h"
#include "../../../State.h"

const char * DE::TestComponent::GetComponentName() const {
    return "测试组件";
}

void DE::TestComponent::Start(void *appstate) {
    Log::Debug("组件初始化.");
}

void DE::TestComponent::Iterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    Log::Debug(std::string("组件调用:") + std::format("{:.2f}", state->current_time));
    time++;
}
