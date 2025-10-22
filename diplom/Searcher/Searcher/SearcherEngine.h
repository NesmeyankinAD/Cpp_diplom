#pragma once
#include "SearcherPostgresDatabase.h"
#include "SearchResult.h"
#include <vector>
#include <string>

class SearcherEngine {
public:
    explicit SearcherEngine(SearcherPostgresDatabase* db) : db_(db) {}
    std::vector<SearchResult> search(const std::string& query, size_t limit = 10);

private:
    SearcherPostgresDatabase* db_;
    std::vector<std::string> tokenize(const std::string& q);
    bool resolveWordIds(const std::vector<std::string>& tokens, std::vector<int>& ids);
    std::vector<SearchResult> fetchResults(const std::vector<int>& wordIds, size_t limit);
};