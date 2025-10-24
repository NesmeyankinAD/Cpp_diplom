#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>

class ConfigManager
{
private:
    // Внутреннее хранение настроек
    struct Data
    {
        std::string dbHost;
        int dbPort;
        std::string dbName;
        std::string dbUser;
        std::string dbPassword;
        std::string startPage;
        int recursionDepth;
        int serverPort;
        int crawlDelayMs; //задержка между запросами (мс)
    } data_;

public:
    explicit ConfigManager(const std::string& filename);

    // Геттеры
    std::string getDBHost() const;
    int getDBPort() const;
    std::string getDBName() const;
    std::string getDBUser() const;
    std::string getDBPassword() const;
    std::string getStartPage() const;
    int getRecursionDepth() const;
    int getServerPort() const;
    int getCrawlDelayMs() const; 
private:
    void validate() const;
};

#endif // CONFIG_MANAGER_H