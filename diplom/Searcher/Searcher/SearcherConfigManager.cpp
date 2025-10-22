#include "SearcherConfigManager.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <stdexcept>

SearcherConfigManager::SearcherConfigManager(const std::string& filename)
{
    using boost::property_tree::ptree;

    ptree pt;
    // Чтение ini-файла.
    boost::property_tree::ini_parser::read_ini(filename, pt);

    data_.dbHost = pt.get<std::string>("database.host", "localhost");
    data_.dbPort = pt.get<int>("database.port", 5432);
    data_.dbName = pt.get<std::string>("database.name", "diplom");
    data_.dbUser = pt.get<std::string>("database.user", "postgres");
    data_.dbPassword = pt.get<std::string>("database.password", "");
    data_.serverPort = pt.get<int>("server.port", 8080);

    validate();
}

std::string SearcherConfigManager::getDBHost() const { return data_.dbHost; }
int SearcherConfigManager::getDBPort() const { return data_.dbPort; }
std::string SearcherConfigManager::getDBName() const { return data_.dbName; }
std::string SearcherConfigManager::getDBUser() const { return data_.dbUser; }
std::string SearcherConfigManager::getDBPassword() const { return data_.dbPassword; }
int SearcherConfigManager::getServerPort() const { return data_.serverPort; }

void SearcherConfigManager::validate() const
{
    // Простейшая валидация значений
    if (data_.serverPort <= 0 || data_.serverPort > 65535)
    {
        throw std::runtime_error("Invalid server port in searcher configuration");
    }
    if (data_.dbPort <= 0 || data_.dbPort > 65535)
    {
        throw std::runtime_error("Invalid database port in configuration");
    }
}