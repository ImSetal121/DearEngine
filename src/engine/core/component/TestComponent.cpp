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

bool DE::TestComponent::Init(void *appstate) {
    Log::Debug("组件初始化.");
    return true;
}

bool DE::TestComponent::Start(void *appstate) {
    return IComponent::Start(appstate);
}

bool DE::TestComponent::Event() {
    return IComponent::Event();
}

bool DE::TestComponent::LogicIterate(void *appstate) {
    auto *state = static_cast<AppState *>(appstate);

    Log::Debug(std::string("组件调用:") + std::format("{:.2f}", state->current_time));
    time++;
    return true;
}

bool DE::TestComponent::RenderIterate(void *appstate, RenderContext* render_context) {
    return IComponent::RenderIterate(appstate, render_context);
}

bool DE::TestComponent::End() {
    return IComponent::End();
}

bool DE::TestComponent::Quit() {
    return IComponent::Quit();
}
