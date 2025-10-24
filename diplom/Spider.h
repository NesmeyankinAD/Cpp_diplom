#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

#include "ConfigManager.h"
#include "IDatabase.h"
#include "HttpLogger.h"

class Spider
{
public:
    // cm - конфигурация, db - интерфейс доступа к БД
    Spider(const ConfigManager& cm, IDatabase* db);

    // Запуск паука
    void start();

private:
    static constexpr int kNumWorkers = 4; // Количество потоков паука

    ConfigManager cm_;
    IDatabase* db_;

    std::string startPage_;
    int maxDepth_;
    int crawlDelayMs_;

    // Очередь задач
    struct Task 
    {
        std::string url;
        int depth;
    };

    std::queue<Task> frontier_;
    std::mutex frontier_mutex_;
    std::condition_variable frontier_cv_;

    // Рабочие потоки
    std::array<std::thread, kNumWorkers> workers_;
    std::atomic<bool> stop_{ false };
    std::atomic<int> activeTasks_{ 0 }; // активные задачи в данный момент

    // Защита от повторного посещения
    std::unordered_set<std::string> visited_;
    std::mutex visited_mutex_;

    // БД
    std::mutex dbMutex_;

    // Вспомогательные методы
    void workerLoop();

    // Очистка БД перед запуском текущего прогона
    void resetDatabaseForCurrentRun();

    // Парсинг и индексирование
    bool fetchPage(const std::string& url, std::string& content, std::vector<std::string>& links);
    std::string stripHtml(const std::string& html);
    std::unordered_map<std::string, int> indexWords(const std::string& text);

    // Работа с документами/словами
    int upsertDocumentId(const std::string& url);
    int upsertWordId(const std::string& word);
    int upsertWordIdNoLock(const std::string& word);
    void storeDocumentWords(int docId, const std::unordered_map<std::string, int>& freqs);

    int upsertDocumentIdNoLock(const std::string& url);
    void storeDocumentWordsNoLock(int docId, const std::unordered_map<std::string, int>& freqs);

    // Вспомогательное
    std::string sqlEscape(const std::string& s);
    std::string extractDomain(const std::string& url) const;
    bool isSameDomain(const std::string& url) const;
};