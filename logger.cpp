#include "logger.h"
#include <windows.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>

bool g_enableLogging = false;

// Gets the path for the log file, placing it next to the executable.
static std::filesystem::path GetLogFilePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path() / "lang_indicator.log";
}

void Log(const std::wstring& message) {
    if (!g_enableLogging) return;

    static const std::filesystem::path logPath = GetLogFilePath();
    std::wofstream logfile(logPath, std::ios_base::app);
    if (!logfile.is_open()) {
        return; // Failed to open log file
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm buf;
    localtime_s(&buf, &in_time_t);

    wchar_t time_buf[100];
    wcsftime(time_buf, 100, L"%Y-%m-%d %H:%M:%S", &buf);

    logfile << L"[" << time_buf << L"] " << message << std::endl;
}
