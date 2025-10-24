﻿#include <iostream>

#include "SearcherConfigManager.h"
#include "SearcherPostgresDatabase.h"
#include "SearcherEngine.h"
#include "SearchResult.h"
#include "HtmlRenderer.h"
#include "HttpServerBeast.h"

#include <boost/asio.hpp>

// Проверка работы
int main_1() 
{
    // Чтение конфигурации
    SearcherConfigManager config("searcher.ini");

    // Формирование строки подключения
    std::string conn =  std::string("host=") + config.getDBHost() +
                        " port=" + std::to_string(config.getDBPort()) +
                        " dbname=" + config.getDBName() +
                        " user=" + config.getDBUser() +
                        " password=" + config.getDBPassword();

    // Подключение к БД (инициализацию схемы отключаем для MVP)
    SearcherPostgresDatabase db(config.getDBHost(),
                                config.getDBPort(),
                                config.getDBName(),
                                config.getDBUser(),
                                config.getDBPassword());

    if (!db.connect(conn)) 
    {
        std::cerr << "Failed to connect to database." << std::endl;
        return 1;
    }

    std::cout << "Connected to DB. Skeleton ready." << std::endl;
    return 0;
}

int main_2() {
    // 1) читаем конфигурацию
    SearcherConfigManager cfg("searcher.ini");

    // 2) формируем строку подключения
    std::string conn = "host=" + cfg.getDBHost() +
                       " port=" + std::to_string(cfg.getDBPort()) +
                       " dbname=" + cfg.getDBName() +
                       " user=" + cfg.getDBUser() +
                       " password=" + cfg.getDBPassword();

    // 3) БД 
    SearcherPostgresDatabase db(cfg.getDBHost(),
                                cfg.getDBPort(),
                                cfg.getDBName(),
                                cfg.getDBUser(),
                                cfg.getDBPassword());

    if (!db.connect(conn)) 
    {
        std::cerr << "DB connect failed." << std::endl;
        return 1;
    }

    // 4) движок поиска и тестовый запрос
    SearcherEngine engine(&db);
    auto results = engine.search("for", 10);

    // 5) вывод результатов в консоль
    for (const auto& r : results) 
    {
        std::cout << r.url << " -> " << r.score << std::endl;
    }

    return 0;
}
// Проверка работы


int main() 
{
    // 1) читаем конфигурацию
    SearcherConfigManager cfg("searcher.ini");

    // 2) формируем строку подключения к БД
    std::string conn =  "host=" + cfg.getDBHost() +
                        " port=" + std::to_string(cfg.getDBPort()) +
                        " dbname=" + cfg.getDBName() +
                        " user=" + cfg.getDBUser() +
                        " password=" + cfg.getDBPassword();

    // 3) БД
    SearcherPostgresDatabase db(cfg.getDBHost(),
                                cfg.getDBPort(),
                                cfg.getDBName(),
                                cfg.getDBUser(),
                                cfg.getDBPassword());

    if (!db.connect(conn)) 
    {
        std::cerr << "DB connect failed." << std::endl;
        return 1;
    }

    HtmlRenderer htmlRenderer;
    SearcherEngine engine(&db);

    // 4) HTTP сервер на Boost.Beast
    boost::asio::io_context ioc;
    unsigned short httpPort = static_cast<unsigned short>(cfg.getServerPort()); // например 8080

    HttpServerBeast server(ioc, httpPort, &engine, &htmlRenderer);

    // 5) запуск сервера 
    server.run();

    return 0;
}