#include "ConfigManager.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <stdexcept>

ConfigManager::ConfigManager(const std::string& filename)
{
    // Чтение ini-файла.
    using boost::property_tree::ptree;
    ptree pt;
    
    boost::property_tree::ini_parser::read_ini(filename, pt);

    data_.dbHost = pt.get<std::string>("database.host", "localhost");
    data_.dbPort = pt.get<int>("database.port", 5432);
    data_.dbName = pt.get<std::string>("database.name", "searchdb");
    data_.dbUser = pt.get<std::string>("database.user", "user");
    data_.dbPassword = pt.get<std::string>("database.password", "");
    data_.startPage = pt.get<std::string>("spider.startPage", "http://example.com");
    data_.recursionDepth = pt.get<int>("spider.recursionDepth", 2);
    data_.serverPort = pt.get<int>("server.port", 8080);
    data_.crawlDelayMs = pt.get<int>("spider.crawlDelayMs", 1000);

    validate();
}

std::string ConfigManager::getDBHost() const     { return data_.dbHost; }
int ConfigManager::getDBPort() const             { return data_.dbPort; }
std::string ConfigManager::getDBName() const     { return data_.dbName; }
std::string ConfigManager::getDBUser() const     { return data_.dbUser; }
std::string ConfigManager::getDBPassword() const { return data_.dbPassword; }
std::string ConfigManager::getStartPage() const  { return data_.startPage; }
int ConfigManager::getRecursionDepth() const     { return data_.recursionDepth; }
int ConfigManager::getServerPort() const         { return data_.serverPort; }
int ConfigManager::getCrawlDelayMs() const       { return data_.crawlDelayMs; }

void ConfigManager::validate() const
{
    // Валидация значений
    if (data_.serverPort <= 0 || data_.serverPort > 65535)
    {
        throw std::runtime_error("Invalid server port in configuration");
    }
    if (data_.recursionDepth < 1)
    {
        throw std::runtime_error("RecursionDepth must be >= 1");
    }
    if (data_.dbPort <= 0 || data_.dbPort > 65535)
    {
        throw std::runtime_error("Invalid database port in configuration");
    }
}