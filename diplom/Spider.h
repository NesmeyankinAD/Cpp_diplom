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

class Spider
{
public:
    Spider(const ConfigManager& cm, IDatabase* db);

    // Запуск паука
    void start();

private:
    static constexpr int kNumWorkers = 4; // фиксированное число потоков

    ConfigManager cm_;
    IDatabase* db_;

    std::string startPage_;
    int maxDepth_;

    // Задача в очереди frontier
    struct Task {
        std::string url;
        int depth;
    };

    // Потокобезопасная очередь задач
    std::queue<Task> frontier_;
    std::mutex frontier_mutex_;
    std::condition_variable frontier_cv_;

    // Потоки-работники
    std::array<std::thread, kNumWorkers> workers_;
    std::atomic<bool> stop_{ false };
    std::atomic<int> activeTasks_{ 0 }; // активные задачи

    // Дедупликация посещённых страниц
    std::unordered_set<std::string> visited_;
    std::mutex visited_mutex_;

    // БД: простая защита доступа
    std::mutex dbMutex_;

    // Основные приватные методы
    void workerLoop();
    void resetDatabaseForCurrentRun();

    // Хелперы
    bool fetchPage(const std::string& url, std::string& content, std::vector<std::string>& links);
    std::string stripHtml(const std::string& html);
    std::unordered_map<std::string, int> indexWords(const std::string& text);

    int upsertDocumentId(const std::string& url);
    int upsertWordId(const std::string& word);
    int upsertWordIdNoLock(const std::string& word);
    void storeDocumentWords(int docId, const std::unordered_map<std::string, int>& freqs);

    // Новые приватные методы для реализации безопасной записи под одной блокировкой
    int upsertDocumentIdNoLock(const std::string& url);
    void storeDocumentWordsNoLock(int docId, const std::unordered_map<std::string, int>& freqs);

    // Вспомогательные функции
    std::string sqlEscape(const std::string& s);
    std::string extractDomain(const std::string& url) const;
    bool isSameDomain(const std::string& url) const;
};