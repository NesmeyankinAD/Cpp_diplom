#pragma once
#include <string>

class HttpClient {
public:
    // ���������� true, ���� ������� ��������� ��������, content �����������
    static bool fetch(const std::string& url, std::string& content);
};