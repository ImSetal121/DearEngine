//
// Created by ImSetal on 2026/2/22.
//

#include "AssetManagerWindow.h"

#include "imgui.h"
#include "../Engine.h"
#include "../../State.h"

namespace DE {

    AssetManagerWindow::AssetManagerWindow() {
    }

    bool AssetManagerWindow::Init(void *appstate) {
        return IEngineWindow::Init(appstate);
    }

    bool AssetManagerWindow::Event() {
        return IEngineWindow::Event();
    }

    bool AssetManagerWindow::LogicIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_engine_window = this;

        ImGui::End();
        return IEngineWindow::LogicIterate(appstate);
    }

    bool AssetManagerWindow::RenderIterate(void *appstate) {
        return IEngineWindow::RenderIterate(appstate);
    }

    bool AssetManagerWindow::Quit() {
        return IEngineWindow::Quit();
    }
} // DE