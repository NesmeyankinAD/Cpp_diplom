#include "HtmlRenderer.h"
#include <sstream>


// Функция экранирования HTML-символов
std::string escapeHtml(const std::string& s) 
{
    std::string out;

    for (char c : s) 
    {
        switch (c) 
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;";  break;
        case '>': out += "&gt;";  break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c;
        }
    }
    return out;
}

// Шаблон заголовков и базовая стилизация
const char* baseCss = R"(
    <style>
        body { font-family: Arial, sans-serif; margin: 2rem; }
        h1 { font-size: 1.6rem; }
        .result { margin: .5rem 0; padding: .5rem; border-bottom: 1px solid #ddd; }
        .url { color: #1a0dab; text-decoration: none; }
        .score { color: #555; font-size: .9em; }
        .search-form { margin-bottom: 1rem; }
    </style>
)";


std::string HtmlRenderer::render(const std::vector<SearchResult>& results, const std::string& query) 
{
    std::ostringstream oss;

    oss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Search</title>"
        << baseCss
        << "</head><body>"
        << "<h1>Search</h1>"
        << "<form class=\"search-form\" method=\"get\" action=\"/search\">"
        << "<input type=\"text\" name=\"q\" value=\"" << escapeHtml(query) << "\" size=\"40\"/>"
        << "<button type=\"submit\">Search</button>"
        << "</form>";

    if (!results.empty()) 
    {
        oss << "<div class=\"results\">";

        for (const auto& r : results) {
            oss << "<div class=\"result\">"
                << "<a class=\"url\" href=\"" << escapeHtml(r.url) << "\">" << escapeHtml(r.url) << "</a>"
                << "<div class=\"score\">Score: " << r.score << "</div>"
                << "</div>";
        }

        oss << "</div>";
    }
    else 
    {
        oss << "<p>No results found.</p>";
    }

    oss << "</body></html>";

    return oss.str();
}

std::string HtmlRenderer::renderIndex() 
{
    std::ostringstream oss;

    oss << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Search</title>"
        << baseCss
        << "</head><body>"
        << "<h1>Search</h1>"
        << "<form class=\"search-form\" method=\"get\" action=\"/search\">"
        << "<input type=\"text\" name=\"q\" placeholder=\"Enter query\" size=\"40\"/>"
        << "<button type=\"submit\">Search</button>"
        << "</form>"
        << "</body></html>";

    return oss.str();
}