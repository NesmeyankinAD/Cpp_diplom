#include "SearcherConfigManager.h"
#include "SearcherPostgresDatabase.h"
#include <iostream>

int main_1() 
{
    // ������ ������������
    SearcherConfigManager config("searcher.ini");

    // ������������ ������ �����������
    std::string conn =
        std::string("host=") + config.getDBHost() +
        " port=" + std::to_string(config.getDBPort()) +
        " dbname=" + config.getDBName() +
        " user=" + config.getDBUser() +
        " password=" + config.getDBPassword();

    // ����������� � �� (������������� ����� ��������� ��� MVP)
    SearcherPostgresDatabase db(config.getDBHost(),
        config.getDBPort(),
        config.getDBName(),
        config.getDBUser(),
        config.getDBPassword());

    if (!db.connect(conn)) {
        std::cerr << "Failed to connect to database." << std::endl;
        return 1;
    }

    // ��������� ������ � HTTP-������ ����� ���������� ����� �����
    // ��������:
    // SearchEngine engine(&db);
    // HttpServer server(ioc, static_cast<unsigned short>(config.getServerPort()), engine);
    // server.run();

    std::cout << "Connected to DB. MVP skeleton ready." << std::endl;
    return 0;
}


#include "SearcherConfigManager.h"
#include "SearcherPostgresDatabase.h"
#include "SearcherEngine.h"
#include "SearchResult.h"

#include <iostream>

int main_2() {
    // 1) ������ ������������
    SearcherConfigManager cfg("searcher.ini");

    // 2) ��������� ������ �����������
    std::string conn =
        "host=" + cfg.getDBHost() +
        " port=" + std::to_string(cfg.getDBPort()) +
        " dbname=" + cfg.getDBName() +
        " user=" + cfg.getDBUser() +
        " password=" + cfg.getDBPassword();

    // 3) �� (������������� ����� �� �����)
    SearcherPostgresDatabase db(cfg.getDBHost(),
        cfg.getDBPort(),
        cfg.getDBName(),
        cfg.getDBUser(),
        cfg.getDBPassword());

    if (!db.connect(conn)) {
        std::cerr << "DB connect failed." << std::endl;
        return 1;
    }

    // 4) ������ ������ � �������� ������
    SearcherEngine engine(&db);
    auto results = engine.search("for", 10);

    // 5) ����� ����������� � �������
    for (const auto& r : results) {
        std::cout << r.url << " -> " << r.score << std::endl;
    }

    return 0;
}

#include "SearcherConfigManager.h"
#include "SearcherPostgresDatabase.h"
#include "SearcherEngine.h"
#include "SearchResult.h"
#include "HtmlRenderer.h"
#include "HttpServerBeast.h"

#include <boost/asio.hpp>
#include <iostream>

int main() {
    // 1) ������ ������������
    SearcherConfigManager cfg("searcher.ini");

    // 2) ��������� ������ ����������� � ��
    std::string conn =
        "host=" + cfg.getDBHost() +
        " port=" + std::to_string(cfg.getDBPort()) +
        " dbname=" + cfg.getDBName() +
        " user=" + cfg.getDBUser() +
        " password=" + cfg.getDBPassword();

    // 3) �� � ������
    SearcherPostgresDatabase db(cfg.getDBHost(),
        cfg.getDBPort(),
        cfg.getDBName(),
        cfg.getDBUser(),
        cfg.getDBPassword());

    if (!db.connect(conn)) {
        std::cerr << "DB connect failed." << std::endl;
        return 1;
    }

    HtmlRenderer htmlRenderer;
    SearcherEngine engine(&db);

    // 4) HTTP ������ �� Boost.Beast
    boost::asio::io_context ioc;
    unsigned short httpPort = static_cast<unsigned short>(cfg.getServerPort()); // �������� 8080

    HttpServerBeast server(ioc, httpPort, &engine, &htmlRenderer);

    // 5) ������ ������� (�����������)
    server.run();

    return 0;
}