//
// Created by ImSetal on 2026/2/22.
//

#include "AssetManagerSubWindow.h"

#include "imgui.h"
#include "../EngineEditor.h"
#include "../../../State.h"

namespace DE {

    AssetManagerSubWindow::AssetManagerSubWindow() {
    }

    bool AssetManagerSubWindow::Init(void *appstate) {
        return IEditorSubWindow::Init(appstate);
    }

    bool AssetManagerSubWindow::Event() {
        return IEditorSubWindow::Event();
    }

    bool AssetManagerSubWindow::EditorIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused())
            state->focused_editor_subwindow = this;

        ImGui::End();
        return IEditorSubWindow::EditorIterate(appstate);
    }

    bool AssetManagerSubWindow::RenderIterate(void *appstate) {
        return IEditorSubWindow::RenderIterate(appstate);
    }

    bool AssetManagerSubWindow::Quit() {
        return IEditorSubWindow::Quit();
    }
} // DE