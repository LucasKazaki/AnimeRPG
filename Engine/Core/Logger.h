#pragma once

#include <fstream>
#include <string>

namespace Astral::Core {

class Logger {
public:
    explicit Logger(const std::string& filePath);
    ~Logger();

    void Info(const std::string& message);
    bool IsOpen() const;

private:
    std::ofstream file_;
};

} // namespace Astral::Core
