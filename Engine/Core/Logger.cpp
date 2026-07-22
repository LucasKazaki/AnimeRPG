#include "Engine/Core/Logger.h"

#include <chrono>
#include <ctime>
#include <iostream>

namespace Astral::Core {

Logger::Logger(const std::string& filePath) : file_(filePath, std::ios::app) {}
Logger::~Logger() = default;

void Logger::Info(const std::string& message) {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime{};
    localtime_s(&localTime, &now);
    char timestamp[32]{};
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTime);
    const std::string line = std::string("[") + timestamp + "] " + message;
    std::cout << line << std::endl;
    if (file_.is_open()) {
        file_ << line << std::endl;
        file_.flush();
    }
}

bool Logger::IsOpen() const { return file_.is_open(); }

} // namespace Astral::Core
