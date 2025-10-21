#pragma once
#include <string>

class HttpClient {
public:
    // Возвращает true, если удалось загрузить страницу, content заполняется
    static bool fetch(const std::string& url, std::string& content);
};