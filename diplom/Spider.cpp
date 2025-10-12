#include "Spider.h"

#include <sstream>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <cctype>

#include "HttpClient.h" // ������ ��� HTTP-�������� (Boost.Beast)

Spider::Spider(const ConfigManager& cm, IDatabase* db)
    : cm_(cm), db_(db),
    startPage_(cm.getStartPage()),
    maxDepth_(cm.getRecursionDepth())
{
}

void Spider::start() {
    if (!db_) return;

    // ����������� � �� (���� ���������)
    if (!db_->isConnected()) {
        if (!db_->connect("")) {
            // �� ������� ������������
            return;
        }
    }

    // ������� BFS-�������: ���� URL � ������� �������
    std::vector<std::pair<std::string, int>> queue;
    std::unordered_set<std::string> visited;

    queue.push_back({ startPage_, 0 });
    visited.insert(startPage_);

    for (size_t i = 0; i < queue.size(); ++i) {
        const auto& [url, depth] = queue[i];
        std::string content;
        std::vector<std::string> links;
        if (!fetchPage(url, content, links)) {
            continue;
        }

        // ���������� ����������
        auto wordFreq = indexWords(stripHtml(content));
        int docId = upsertDocumentId(url);
        if (docId > 0) {
            storeDocumentWords(docId, wordFreq);
        }

        // ��������� ������ � �������, ����������� �������
        if (depth < maxDepth_) {
            for (auto& link : links) {
                if (!visited.count(link) && isSameDomain(link)) {
                    visited.insert(link);
                    queue.push_back({ link, depth + 1 });
                }
            }
        }
    }
}

bool Spider::fetchPage(const std::string& url, std::string& content, std::vector<std::string>& links) {
    content.clear();
    links.clear();

    // ���������� ������� HTTP GET ����� ������
    if (!HttpClient::fetch(url, content)) {
        return false;
    }

    // ���������� ������ (���� �������� ��������)
    // ��� ������������� raw-�����, �������� ������ ����������
    std::regex aHref("<a\\s+(?:[^>]*?\\s+)?href=\\\"([^\\\"]+)\\\"", std::regex_constants::icase);
    for (std::sregex_iterator it(content.begin(), content.end(), aHref);
        it != std::sregex_iterator();
        ++it)
    {
        std::string href = (*it)[1].str();
        if (!href.empty()) {
            if (href.find("http://") == 0 || href.find("https://") == 0) {
                links.push_back(href);
            }
            else {
                // ������� ��������� ������������� ������
                std::string domain = extractDomain(startPage_);
                if (!href.empty() && href[0] == '/') {
                    links.push_back(domain + href);
                }
                else {
                    // ������������� ����: ����� + "/" + href
                    links.push_back(domain + "/" + href);
                }
            }
        }
    }

    return !content.empty();
}

std::string Spider::stripHtml(const std::string& html) {
    // ������� �������� �����
    std::string text;
    bool inTag = false;
    for (char ch : html) {
        if (ch == '<') { inTag = true; continue; }
        if (ch == '>') { inTag = false; continue; }
        if (!inTag) text += ch;
    }
    // ������ �������� �������� �� �������
    for (char& c : text) {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    }
    return text;
}

std::unordered_map<std::string, int> Spider::indexWords(const std::string& text) {
    std::unordered_map<std::string, int> freq;
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        // ������� ������������ �������� �� ����� � ���������� � ������� ��������
        size_t l = 0;
        size_t r = word.size();
        while (l < r && !std::isalnum(static_cast<unsigned char>(word[l]))) ++l;
        while (r > l && !std::isalnum(static_cast<unsigned char>(word[r - 1]))) --r;
        if (l >= r) continue;
        std::string w = word.substr(l, r - l);
        for (auto& ch : w) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        if (w.size() < 3 || w.size() > 32) continue;
        freq[w] += 1;
    }
    return freq;
}

int Spider::upsertDocumentId(const std::string& url) {
    // �������/���������� ��������� � ������� id
    std::string sql = "INSERT INTO documents (url) VALUES ('" + sqlEscape(url) + "') "
        "ON CONFLICT (url) DO UPDATE SET url = EXCLUDED.url "
        "RETURNING id;";
    std::string res = db_->query(sql);
    if (res.empty()) return -1;
    std::stringstream ss(res);
    int id;
    if (ss >> id) return id;
    return -1;
}

int Spider::upsertWordId(const std::string& word) {
    std::string sql = "INSERT INTO words (word) VALUES ('" + sqlEscape(word) + "') "
        "ON CONFLICT (word) DO UPDATE SET word = EXCLUDED.word "
        "RETURNING id;";
    std::string res = db_->query(sql);
    if (res.empty()) return -1;
    std::stringstream ss(res);
    int id;
    if (ss >> id) return id;
    return -1;
}

void Spider::storeDocumentWords(int docId, const std::unordered_map<std::string, int>& freqs) {
    for (const auto& [word, freq] : freqs) {
        int wordId = upsertWordId(word);
        if (wordId <= 0) continue;
        std::stringstream sql;
        sql << "INSERT INTO document_words (document_id, word_id, frequency) "
            << "VALUES (" << docId << ", " << wordId << ", " << freq << ") "
            << "ON CONFLICT (document_id, word_id) "
            << "DO UPDATE SET frequency = document_words.frequency + EXCLUDED.frequency;";
        db_->query(sql.str());
    }
}

std::string Spider::sqlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}

std::string Spider::extractDomain(const std::string& url) const {
    // ������� ����������: ���������� �������� + ����� (� ����������� ����)
    std::string tmp = url;
    const std::string http = "http://";
    const std::string https = "https://";
    size_t pos = 0;
    if (tmp.compare(0, http.size(), http) == 0) pos = http.size();
    else if (tmp.compare(0, https.size(), https) == 0) pos = https.size();
    else pos = 0;

    size_t slash = tmp.find('/', pos);
    if (slash == std::string::npos) return tmp;
    return tmp.substr(0, slash);
}

bool Spider::isSameDomain(const std::string& url) const {
    if (startPage_.empty()) return true;
    std::string domain0 = extractDomain(startPage_);
    std::string domain1 = extractDomain(url);
    return domain0 == domain1;
}