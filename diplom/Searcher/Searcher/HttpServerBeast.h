#pragma once

#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <string>

class HtmlRenderer;
class SearcherEngine;

class HttpServerBeast {
public:
    HttpServerBeast(boost::asio::io_context& ioc,
        unsigned short port,
        SearcherEngine* engine,
        HtmlRenderer* renderer);

    // Блокирующий запуск сервера (при вызове слушает и обрабатывает соединения)
    void run();

private:
    boost::asio::io_context& ioc_;
    unsigned short port_;
    boost::asio::ip::tcp::acceptor acceptor_;
    SearcherEngine* engine_;
    HtmlRenderer* renderer_;
};