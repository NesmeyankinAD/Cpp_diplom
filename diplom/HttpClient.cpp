#include "HttpClient.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <string>
#include <cstdlib>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

bool HttpClient::fetch(const std::string& url, std::string& content) {
    try {
        // Разбор URL на схему://host[:port]/path
        // Поддерживаем http, default port 80
        std::string scheme = "http";
        std::string host;
        std::string target = "/";
        unsigned short port = 80;

        std::string tmp = url;
        // Небольшой парсер URL
        size_t pos = tmp.find("://");
        if (pos != std::string::npos) {
            scheme = tmp.substr(0, pos);
            size_t host_start = pos + 3;
            size_t host_end = tmp.find('/', host_start);
            if (host_end == std::string::npos) host_end = tmp.size();
            host = tmp.substr(host_start, host_end - host_start);
            if (host_end < tmp.size()) target = tmp.substr(host_end);
            else target = "/";
        }
        else {
            // если URL не содержит протокола, считаем как http://host/...
            size_t host_end = tmp.find('/');
            host = tmp.substr(0, host_end);
            if (host_end != std::string::npos) target = tmp.substr(host_end);
        }

        // Опционально извлечь порт из host (если указан)
        size_t colon = host.find(':');
        if (colon != std::string::npos) {
            port = static_cast<unsigned short>(std::stoi(host.substr(colon + 1)));
            host = host.substr(0, colon);
        }

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        tcp::resolver::results_type results = resolver.resolve(host, std::to_string(port));
        beast::tcp_stream stream(ioc);
        stream.connect(results);

        http::request<http::string_body> req{ http::verb::get, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        content = res.body();
        // закрываем соединение
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        return res.result() == http::status::ok;
    }
    catch (...) {
        return false;
    }
}