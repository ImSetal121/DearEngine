//
// Created by ImSetal on 2026/2/8.
//

#include "ConsoleSubWindow.h"

#include "imgui.h"
#include "../../../State.h"
#include "../../core/Log.h"

namespace DE {
    ConsoleSubWindow::ConsoleSubWindow() = default;

    /** 窗口标题，用于 ImGui::Begin(title, ...) */
    const char* ConsoleSubWindow::Title() const {
        return "控制台";
    }

    bool ConsoleSubWindow::Init(void *appstate) {
        return IEditorSubWindow::Init(appstate);
    }

    bool ConsoleSubWindow::Event() {
        return IEditorSubWindow::Event();
    }

    /** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
    bool ConsoleSubWindow::LogicIterate(void *appstate) {
        if (!open) return false;
        ImGui::Begin(Title());
        // 检查是否为焦点窗口
        auto state = static_cast<AppState*>(appstate);
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            state->focused_editor_subwindow = this;

        if (ImGui::Button("清空"))
            DE::Log::Clear();

        ImGui::SameLine();
        ImGui::Separator();

        if (ImGui::BeginChild("LogRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            const auto& entries = DE::Log::GetEntries();
            for (const auto& entry : entries) {
                ImVec4 color;
                switch (entry.level) {
                    case DE::LogLevel::DEBUG:   color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break;
                    case DE::LogLevel::INFO:    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                    case DE::LogLevel::WARNING: color = ImVec4(1.0f, 0.9f, 0.0f, 1.0f); break;
                    case DE::LogLevel::ERROR:   color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
                    case DE::LogLevel::FATAL:   color = ImVec4(0.8f, 0.0f, 0.0f, 1.0f); break;
                }
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(entry.message.c_str());
                ImGui::PopStyleColor();
            }
            if (entries.size() > 0)
                ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();

        return true;
    }

    bool ConsoleSubWindow::RenderIterate(void *appstate) {
        return IEditorSubWindow::RenderIterate(appstate);
    }

    bool ConsoleSubWindow::Quit() {
        return IEditorSubWindow::Quit();
    }
}
