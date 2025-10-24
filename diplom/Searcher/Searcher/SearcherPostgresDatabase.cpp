#include "SearcherPostgresDatabase.h"
#include <sstream>
#include <iostream>
#include <pqxx/pqxx>

SearcherPostgresDatabase::SearcherPostgresDatabase(const std::string& host,
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
    connected_(false)
    {}

bool SearcherPostgresDatabase::connect(const std::string& connectionString)
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
    }
    catch (...)
    {
        connected_ = false;
    }

    return connected_;
}

void SearcherPostgresDatabase::disconnect()
{
    conn_.reset();
    connected_ = false;
}

bool SearcherPostgresDatabase::isConnected() const
{
    return connected_ && conn_ && conn_->is_open();
}

std::string SearcherPostgresDatabase::query(const std::string& sql)
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

pqxx::result SearcherPostgresDatabase::execQuery(const std::string& sql)
{
    if (!isConnected()) return pqxx::result();
    try
    {
        pqxx::work txn(*conn_);
        pqxx::result res = txn.exec(sql);
        txn.commit();
        return res;
    }
    catch (...)
    {
        return pqxx::result();
    }
}