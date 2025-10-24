#include "ConfigManager.h"
#include "DatabaseFactory.h"
#include <iostream>
#include "IDatabase.h"
#include <memory>
#include "Spider.h"

// Проверка работы
int main_1() 
{
    ConfigManager cm("config.ini");

    auto db = DatabaseFactory::create(cm);

    if (!db->connect("")) 
    {
        return 1;
    }

    auto res = db->query("SELECT 1");
    // std::cout << res << std::endl;

    db->disconnect();
    return 0;
}
int main_2() 
{
    try 
    {
        const std::string iniPath = "config.ini";

        // Создание конфигурации и экземпляра БД через фабрику
        ConfigManager cm(iniPath);
        std::unique_ptr<IDatabase> db = DatabaseFactory::create(cm);

        if (!db) 
        {
            std::cerr << "Failed to create database instance." << std::endl;
            return 1;
        }

        // Подключение (пустая строка означает использование полей ConfigManager)
        if (!db->connect("")) 
        {
            std::cerr << "Database connection failed." << std::endl;
            return 1;
        }

        // Пример простого запроса: проверить работу БД
        std::string resp = db->query("SELECT 'OK' AS status;");
        std::cout << "DB response:\n" << resp << std::endl;

        db->disconnect();
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
// Проверка работы



int main() 
{
    try {
        const std::string iniPath = "config.ini";

        // Конфигурация
        ConfigManager cm(iniPath);
        std::unique_ptr<IDatabase> db = DatabaseFactory::create(cm);

        if (!db) 
        {
            std::cerr << "Failed to create database instance." << std::endl;
            return 1;
        }

        if (!db->connect("")) 
        {
            std::cerr << "Database connection failed." << std::endl;
            return 1;
        }

        // Создаем паука и запускаем crawl
        Spider spider(cm, db.get());
        spider.start(); 

        db->disconnect();
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}