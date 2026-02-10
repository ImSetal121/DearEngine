//
// Created by ImSetal on 2026/2/10.
//

#include "Log.h"

namespace DE {

    static const size_t kMaxLogEntries = 500;

    std::vector<LogEntry>& Log::GetMutableEntries() {
        static std::vector<LogEntry> s_entries;
        return s_entries;
    }

    const std::vector<LogEntry>& Log::GetEntries() {
        return GetMutableEntries();
    }

    void Log::Clear() {
        GetMutableEntries().clear();
    }

    void Log::PushEntry(LogEntry entry) {
        auto& entries = GetMutableEntries();
        entries.push_back(std::move(entry));
        if (entries.size() > kMaxLogEntries)
            entries.erase(entries.begin());
    }
    
    void Log::Debug(std::string message) {
        PushEntry({ LogLevel::DEBUG, message });
        printf("[Debug]%s\n", message.c_str());
    }

    void Log::Info(std::string message) {
        PushEntry({ LogLevel::INFO, message });
        printf("[Info]%s\n", message.c_str());
    }

    void Log::Warning(std::string message) {
        PushEntry({ LogLevel::WARNING, message });
        printf("[Warning]%s\n", message.c_str());
    }

    void Log::Error(std::string message) {
        PushEntry({ LogLevel::ERROR, message });
        printf("[Error]%s\n", message.c_str());
    }

    void Log::Fatal(std::string message) {
        PushEntry({ LogLevel::FATAL, message });
        printf("[Fatal]%s\n", message.c_str());
    }

} // DE