#include "Spider.h"

#include <iostream>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cctype>

#include "HttpClient.h"   // Используем для загрузки страниц (HTTP/HTTPS)
#include "HttpLogger.h"   // Логирование


Spider::Spider(const ConfigManager& cm, IDatabase* db)
    : cm_(cm),
    db_(db),
    startPage_(cm.getStartPage()),
    maxDepth_(cm.getRecursionDepth()),
    crawlDelayMs_(cm.getCrawlDelayMs()){}

// Запуск паука
void Spider::start() 
{
    if (!db_) 
    {
        HttpLogger::log("Spider", "DB pointer is null. Aborting.");
        return;
    }

    // Подключение к БД
    if (!db_->isConnected()) 
    {
        if (!db_->connect("")) 
        {
            HttpLogger::log("Spider", "DB connect failed.");
            return;
        }
    }

    HttpLogger::log("Spider", "DB connected.");

    // Очистка БД перед прогоном
    resetDatabaseForCurrentRun();
    HttpLogger::log("Spider", "Database reset for current run.");

    // Начальная задача
    {
        std::lock_guard<std::mutex> lock(frontier_mutex_);
        frontier_.push({ startPage_, 0 });
    }
    visited_.insert(startPage_);

    stop_ = false;
    activeTasks_ = 0;

    // Запуск рабочих потоков
    for (size_t i = 0; i < kNumWorkers; ++i) 
    {
        workers_[i] = std::thread(&Spider::workerLoop, this);
    }

    // Ожидание завершения прогона
    while (true) 
    {
        {
            std::lock_guard<std::mutex> lock(frontier_mutex_);
            if (frontier_.empty() && activeTasks_ == 0) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Остановка и ожидание потоков
    stop_ = true;
    frontier_cv_.notify_all();
    for (auto& th : workers_) 
    {
        if (th.joinable()) th.join();
    }

    HttpLogger::log("Spider", "Crawl finished.");
}

bool Spider::fetchPage(const std::string& url, 
                       std::string& content, 
                       std::vector<std::string>& links) 
{
    content.clear();
    links.clear();

    if (!HttpClient::fetch(url, content)) 
    {
        HttpLogger::log("Spider", "FETCH FAILED: " + url);
        return false;
    }

    HttpLogger::log("Spider", "Fetched page: " + url + 
                    " (size=" + std::to_string(content.size()) + ")");

    // Парсинг ссылок
    std::regex aHref(R"(<a\s+(?:[^>]*?\s+)?href=\"([^\"]+)\")", std::regex_constants::icase);

    for (std::sregex_iterator it(content.begin(), content.end(), aHref);
        it != std::sregex_iterator();
        ++it)
    {
        std::string href = (*it)[1].str();

        if (!href.empty()) 
        {
            if (href.find("http://") == 0 || href.find("https://") == 0) 
            {
                links.push_back(href);
            }
            else 
            {
                std::string domain = extractDomain(startPage_);

                if (!href.empty() && href[0] == '/'){links.push_back(domain + href);}
                else                                {links.push_back(domain + "/" + href);}
            }
        }
    }

    HttpLogger::log("Spider", "Links found on " + url + ": " + std::to_string(links.size()));
    return !content.empty();
}

std::string Spider::stripHtml(const std::string& html) 
{
    std::string text;
    bool inTag = false;

    for (char ch : html) 
    {
        if (ch == '<') { inTag = true; continue; }
        if (ch == '>') { inTag = false; continue; }
        if (!inTag) text += ch;
    }

    // Нормализация
    for (char& c : text) 
    {
        if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    }
    return text;
}

std::unordered_map<std::string, int> Spider::indexWords(const std::string& text) 
{
    std::unordered_map<std::string, int> freq;
    std::stringstream ss(text);
    std::string word;

    while (ss >> word) 
    {
        // Очистка по краям и нормализация
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

int Spider::upsertDocumentId(const std::string& url) 
{
    std::string sql = "INSERT INTO documents (url) VALUES ('" + sqlEscape(url) + "') "
        "ON CONFLICT (url) DO UPDATE SET url = EXCLUDED.url "
        "RETURNING id;";

    std::lock_guard<std::mutex> lock(dbMutex_);
    std::string res = db_->query(sql);
 
    if (res.empty()) return -1;

    std::stringstream ss(res);
   
    int id;
    if (ss >> id) return id;
    return -1;
}

int Spider::upsertWordId(const std::string& word) 
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    return upsertWordIdNoLock(word);
}

int Spider::upsertWordIdNoLock(const std::string& word) 
{
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

void Spider::storeDocumentWords(int docId, const std::unordered_map<std::string, int>& freqs) 
{
    // Блокируем запись на протяжении всей обработки одного документа
    std::lock_guard<std::mutex> lock(dbMutex_);

    for (const auto& kv : freqs) 
    {
        int wordId = upsertWordIdNoLock(kv.first);

        if (wordId <= 0) continue;

        std::stringstream sql;
        sql << "INSERT INTO document_words (document_id, word_id, frequency) "
            << "VALUES (" << docId << ", " << wordId << ", " << kv.second << ") "
            << "ON CONFLICT (document_id, word_id) "
            << "DO UPDATE SET frequency = document_words.frequency + EXCLUDED.frequency;";
        
        db_->query(sql.str());
    }
}

std::string Spider::sqlEscape(const std::string& s) 
{
    std::string out;

    out.reserve(s.size() * 2);
    for (char c : s) 
    {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}

void Spider::resetDatabaseForCurrentRun() 
{
    // Очистка текущих данных
    std::string sql = "TRUNCATE TABLE document_words, documents, words RESTART IDENTITY CASCADE;";
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    db_->query(sql);
}

std::string Spider::extractDomain(const std::string& url) const 
{
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

bool Spider::isSameDomain(const std::string& url) const 
{
    if (startPage_.empty()) return true;
    std::string domain0 = extractDomain(startPage_);
    std::string domain1 = extractDomain(url);

    return domain0 == domain1;
}

int Spider::upsertDocumentIdNoLock(const std::string& url) 
{
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

void Spider::storeDocumentWordsNoLock(int docId, const std::unordered_map<std::string, int>& freqs) 
{
    for (const auto& kv : freqs) 
    {
        int wordId = upsertWordIdNoLock(kv.first);
        if (wordId <= 0) continue;

        std::stringstream sql;
        sql << "INSERT INTO document_words (document_id, word_id, frequency) "
            << "VALUES (" << docId << ", " << wordId << ", " << kv.second << ") "
            << "ON CONFLICT (document_id, word_id) "
            << "DO UPDATE SET frequency = document_words.frequency + EXCLUDED.frequency;";
        
        db_->query(sql.str());
    }
}

void Spider::workerLoop() 
{
    while (true) 
    {
        Task task;
        {
            std::unique_lock<std::mutex> lk(frontier_mutex_);

            frontier_cv_.wait(lk, [this] { return stop_ || !frontier_.empty(); });

            if (stop_) break;
            task = frontier_.front();
            frontier_.pop();
            ++activeTasks_;
        }

        HttpLogger::log("Spider", "Processing: " + task.url + " depth=" + std::to_string(task.depth));

        bool ok = false;
        std::string content;
        std::vector<std::string> links;
        ok = fetchPage(task.url, content, links);

        if (ok) 
        {
            std::string text = stripHtml(content);
            auto freqs = indexWords(text);

            int docId = -1;
            {
                std::lock_guard<std::mutex> lock(dbMutex_);

                docId = upsertDocumentIdNoLock(task.url);
                if (docId > 0) 
                {
                    // Записываем словарь документов для данного документа
                    storeDocumentWordsNoLock(docId, freqs);
                }
            }

            if (docId > 0 && task.depth + 1 <= maxDepth_) 
            {
                for (const auto& link : links) 
                {
                    if (!isSameDomain(link)) continue;

                    bool seen = false;
                    {
                        std::lock_guard<std::mutex> vlk(visited_mutex_);
                        if (visited_.count(link)) seen = true;
                    }
                    if (seen) continue;
                    {
                        std::lock_guard<std::mutex> fll(frontier_mutex_);
                        frontier_.push({ link, task.depth + 1 });
                    }
                }
            }
        }
        else 
        {
            HttpLogger::log("Spider", "FAILED to fetch: " + task.url);
        }

        if (crawlDelayMs_ > 0) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(crawlDelayMs_));
            HttpLogger::log("Spider", "Sleeping for " + std::to_string(crawlDelayMs_) + " ms");
        }

        --activeTasks_;
        frontier_cv_.notify_all();
    }
}