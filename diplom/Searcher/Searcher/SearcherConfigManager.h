#ifndef SEARCHER_CONFIG_MANAGER_H
#define SEARCHER_CONFIG_MANAGER_H

#include <string>

class SearcherConfigManager
{
private:
    // ���������� �������� ��������
    struct Data
    {
        std::string dbHost;
        int dbPort;
        std::string dbName;
        std::string dbUser;
        std::string dbPassword;
        int serverPort;
    } data_;

public:
    explicit SearcherConfigManager(const std::string& filename);

    // �������
    std::string getDBHost() const;
    int getDBPort() const;
    std::string getDBName() const;
    std::string getDBUser() const;
    std::string getDBPassword() const;
    int getServerPort() const;

private:
    // ��������� ������������
    void validate() const;
};

#endif // SEARCHER_CONFIG_MANAGER_H