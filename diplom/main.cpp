#include "ConfigManager.h"
#include "DatabaseFactory.h"

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


#include <iostream>
#include "ConfigManager.h"
#include "DatabaseFactory.h"
#include "IDatabase.h"

int main() 
{
    try 
    {
        // Путь к ini-файлу с настройками
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

        // По завершении можно вызвать другие операции (индексация и т.д.)
        db->disconnect();
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}