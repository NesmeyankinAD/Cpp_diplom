#pragma once
#include <string>

class HttpLogger {
public:
    // ������������ � ����������� � �����
    static void log(const std::string& tag, const std::string& message);
};