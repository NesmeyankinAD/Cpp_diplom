#pragma once

#include <string>
#include <vector>
#include "SearchResult.h"

class HtmlRenderer {
public:
    // �������� �������� �����������
    std::string render(const std::vector<SearchResult>& results, const std::string& query);

    // �����������: �������� ������� ������-�������� � ������ ������
    std::string renderIndex();
};