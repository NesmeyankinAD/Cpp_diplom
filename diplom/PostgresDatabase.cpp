#include "PostgresDatabase.h"
#include <sstream>
#include <iostream>
#include <pqxx/pqxx>

PostgresDatabase::PostgresDatabase(const std::string& host,
                                   int port,
                                   const std::string& dbname,
                                   const std::string& user,
                                   const std::string& password)
    : host_(host), 
    port_(port), 
    dbname_(dbname), 
    user_(user), 
    password_(password),
    conn_(nullptr), 
    connected_(false), 
    schemaInitialized_(false) {}

bool PostgresDatabase::connect(const std::string& connectionString)
{
    try
    {
        std::string connInfo = connectionString;

        if (connInfo.empty())
        {
            std::ostringstream oss;

            oss << "host=" << host_
                << " port=" << port_
                << " dbname=" << dbname_
                << " user=" << user_
                << " password=" << password_;

            connInfo = oss.str();
        }

        conn_ = std::make_unique<pqxx::connection>(connInfo);

        connected_ = conn_ && conn_->is_open();

        if (connected_ && !schemaInitialized_) 
        {
            // инициализируем схему БД при первом подключении
            schemaInitialized_ = createTables();
        }
    }
    catch (...)
    {
        connected_ = false;
    }

    return connected_;
}

void PostgresDatabase::disconnect()
{
    conn_.reset();
    connected_ = false;
}

bool PostgresDatabase::isConnected() const
{
    return connected_ && conn_ && conn_->is_open();
}

std::string PostgresDatabase::query(const std::string& sql)
{
    if (!isConnected()) return "";

    try
    {
        pqxx::work txn(*conn_);
        pqxx::result res = txn.exec(sql);

        txn.commit();

        // Если результатов нет, вернуть пустую строку
        if (res.empty()) return "";

        std::ostringstream out;

        for (const auto& row : res)
        {
            for (size_t i = 0; i < res.columns(); ++i)
            {
                const char* s = row[i].c_str();
                out << (s ? s : "");
                if (i + 1 < res.columns()) out << "\t";
            }
            out << "\n";
        }
        return out.str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "DB query failed: " << e.what() << std::endl;
        return "";
    }
}


bool PostgresDatabase::createTables()
{
    if (!isConnected()) return false;

    try 
    {
        pqxx::work txn(*conn_);

        // 1) документы
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS documents (
                id SERIAL PRIMARY KEY,
                url TEXT NOT NULL UNIQUE,
                created_at TIMESTAMP DEFAULT NOW()
            )
        )");

        // 2) слова
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS words (
                id SERIAL PRIMARY KEY,
                word TEXT NOT NULL UNIQUE
            )
        )");

        // 3) связь документы-слова с частотами
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS document_words (
                document_id INTEGER NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
                word_id INTEGER NOT NULL REFERENCES words(id) ON DELETE CASCADE,
                frequency INTEGER NOT NULL,
                PRIMARY KEY (document_id, word_id)
            )
        )");

        // индексы для быстрого поиска по документам и словам
        txn.exec(R"(CREATE INDEX IF NOT EXISTS idx_dw_doc ON document_words(document_id))");
        txn.exec(R"(CREATE INDEX IF NOT EXISTS idx_dw_word ON document_words(word_id))");

        txn.commit();

        return true;
    }
    catch (const std::exception& e) 
    {
        return false;
    }
}