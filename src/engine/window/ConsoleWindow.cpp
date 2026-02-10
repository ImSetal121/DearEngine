//
// Created by ImSetal on 2026/2/8.
//

#include "ConsoleWindow.h"

#include "imgui.h"
#include "../core/Log.h"

ConsoleWindow::ConsoleWindow() = default;

/** 窗口标题，用于 ImGui::Begin(title, ...) */
const char* ConsoleWindow::Title() const {
    return "控制台";
}

/** 每帧调用，内部应包含 ImGui::Begin(Title(), &open) ... ImGui::End() */
void ConsoleWindow::Draw() {
    if (!open) return;
    ImGui::Begin(Title());

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
}