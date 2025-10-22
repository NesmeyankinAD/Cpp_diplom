#pragma once

#include <string>
#include <ostream>

// Результат поиска
struct SearchResult {
    std::string url;
    double       score;
    int          document_id;
};

// Оператор вывода для удобного печатания результатов
std::ostream& operator<<(std::ostream& os, const SearchResult& sr);