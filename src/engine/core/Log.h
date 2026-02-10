//
// Created by ImSetal on 2026/2/10.
//

#ifndef DEARENGINE_LOG_H
#define DEARENGINE_LOG_H
#include <string>
#include <vector>

namespace DE {
    enum class LogLevel {DEBUG, INFO, WARNING, ERROR, FATAL};
    struct LogEntry {
        LogLevel level;
        std::string message;
    };

    class Log {
    public:
        static void Debug(std::string message);
        static void Info(std::string message);
        static void Warning(std::string message);
        static void Error(std::string message);
        static void Fatal(std::string message);

        /** 供控制台窗口读取日志条目（只读） */
        static const std::vector<LogEntry>& GetEntries();
        /** 插入消息 */
        static void PushEntry(LogEntry entry);
        /** 清空日志缓冲区（控制台「清空」按钮可调用） */
        static void Clear();

    private:
        static std::vector<LogEntry>& GetMutableEntries();
    };
} // DE

#endif //DEARENGINE_LOG_H