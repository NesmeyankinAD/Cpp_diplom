#pragma once

#include <string>
#include <vector>
#include "SearchResult.h"

class HtmlRenderer {
public:
    // Рендерит страницу результатов
    std::string render(const std::vector<SearchResult>& results, const std::string& query);

    // Опционально: рендерит простую индекс-страницу с формой поиска
    std::string renderIndex();
};