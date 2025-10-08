#pragma once
#include <string>
#include <memory>
#include "IDatabase.h"
#include <pqxx/pqxx>

class PostgresDatabase : public IDatabase
{
private:
    std::string host_;
    int port_;
    std::string dbname_;
    std::string user_;
    std::string password_;

    std::unique_ptr<pqxx::connection> conn_;
    bool connected_;
    bool schemaInitialized_; 

public:
    PostgresDatabase(const std::string& host,
        int port,
        const std::string& dbname,
        const std::string& user,
        const std::string& password);

    bool connect(const std::string& connectionString) override;
    void disconnect() override;
    bool isConnected() const override;
    std::string query(const std::string& sql) override;

    
    bool createTables();
};