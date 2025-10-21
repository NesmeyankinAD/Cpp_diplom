#pragma once
#include <string>

class HttpLogger {
public:
    // Логгирование с таймштампом и тегом
    static void log(const std::string& tag, const std::string& message);
};