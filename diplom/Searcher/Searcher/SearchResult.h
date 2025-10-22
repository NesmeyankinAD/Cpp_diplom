#pragma once

#include <string>
#include <ostream>

// ��������� ������
struct SearchResult {
    std::string url;
    double       score;
    int          document_id;
};

// �������� ������ ��� �������� ��������� �����������
std::ostream& operator<<(std::ostream& os, const SearchResult& sr);