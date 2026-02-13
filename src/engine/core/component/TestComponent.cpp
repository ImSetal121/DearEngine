//
// Created by ImSetal on 2026/2/12.
//

#include "TestComponent.h"

#include "../Log.h"
#include "../../../Appstate.h"

const char * DE::TestComponent::GetComponentName() const {
    return "Test";
}

void DE::TestComponent::Start(void *appstate) {
    DE::Log::Debug("组件初始化.");
}

void DE::TestComponent::Iterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    DE::Log::Debug(std::string("组件调用:") + std::to_string(state->current_time));
    time++;
}
