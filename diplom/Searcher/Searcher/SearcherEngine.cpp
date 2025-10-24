#include "SearcherEngine.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <pqxx/pqxx>


std::string toLower(std::string s) 
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::vector<std::string> split(const std::string& s) 
{
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string token;

    while (iss >> token) 
    {
        out.push_back(token);
    }
    return out;
}

std::string cleanQuery(const std::string& q) 
{
    std::string cleaned;

    for (char c : q) 
    {
        if (std::isalnum((unsigned char)c)) cleaned.push_back((char)std::tolower((unsigned char)c));
        else cleaned.push_back(' ');
    }
    return cleaned;
}

std::vector<std::string> SearcherEngine::tokenize(const std::string& query) 
{
    std::string clean = cleanQuery(query);
    auto parts = split(clean);
    std::vector<std::string> tokens;

    for (const auto& w : parts) 
    {
        if ((int)w.size() >= 3 && (int)w.size() <= 32) 
        {
            tokens.push_back(w);
            if (tokens.size() >= 4) break;
        }
    }
    return tokens;
}

bool SearcherEngine::resolveWordIds(const std::vector<std::string>& tokens, std::vector<int>& ids) 
{
    ids.clear();
    if (tokens.empty()) return false;

    for (const auto& w : tokens) 
    {
        std::string sql = "SELECT id FROM words WHERE word = '" + w + "'";

        pqxx::result r = db_->execQuery(sql);
        if (r.empty()) 
        {
            return false;
        }

        int id = r[0]["id"].as<int>();
        ids.push_back(id);
    }
    return !ids.empty();
}

std::vector<SearchResult> SearcherEngine::fetchResults(const std::vector<int>& wordIds, size_t limit) 
{
    std::vector<SearchResult> res;
    if (wordIds.empty()) return res;

    std::ostringstream sql;
    sql << "SELECT d.url, SUM(dw.frequency) AS score "
        << "FROM documents d JOIN document_words dw ON dw.document_id = d.id "
        << "WHERE dw.word_id IN (";
            for (size_t i = 0; i < wordIds.size(); ++i) 
            {
                if (i) sql << ",";
                sql << wordIds[i];
            }
    sql << ") GROUP BY d.url ORDER BY score DESC LIMIT " << limit;

    pqxx::result r = db_->execQuery(sql.str());

    for (const auto& row : r) 
    {
        SearchResult sr;

        sr.url = row["url"].c_str();
        sr.score = row["score"].as<double>();
        sr.document_id = 0;
        res.push_back(sr);
    }
    return res;
}

std::vector<SearchResult> SearcherEngine::search(const std::string& query, size_t limit) 
{
    auto tokens = tokenize(query);
    if (tokens.empty()) return {};
    std::vector<int> ids;
    if (!resolveWordIds(tokens, ids) || ids.empty()) return {};

    return fetchResults(ids, limit);
}