#include "HttpClient.h"
#include "HttpLogger.h"

#include <string>
#include <regex>
#include <stdexcept>
#include <fstream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/config.hpp>
#include <openssl/ssl.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

// Функция разбора URL: протокол, хост, путь/цель, порт
static void parseUrl(   const std::string& url,
                        std::string& scheme,
                        std::string& host,
                        std::string& target,
                        std::string& portStr)
{
    // Разбор: протокол://host:порт/path
    std::string s = url;
    scheme = "http";
    portStr.clear();
    host.clear();
    target = "/";

    if (s.rfind("https://", 0) == 0) 
    {
        scheme = "https";
        s = s.substr(8);
    }
    else if (s.rfind("http://", 0) == 0) 
    {
        s = s.substr(7);
    }
    else 
    {
        scheme = "http";
    }

    size_t slash = s.find('/');
    if (slash != std::string::npos) 
    {
        host = s.substr(0, slash);
        target = s.substr(slash);
    }
    else 
    {
        host = s;
        target = "/";
    }

    size_t colon = host.find(':');
    if (colon != std::string::npos) 
    {
        portStr = host.substr(colon + 1);
        host = host.substr(0, colon);
    }
    else 
    {
        portStr = (scheme == "https") ? "443" : "80";
    }
}

// Для https
// Вспомогательная функция: попытка загрузить CACert из локального файла.
// Файл cacert.pem должен находиться в папке проекта (или в рабочей директории при запуске).
static void configureCACertsIfAvailable(ssl::context& ctx)
{
    // 1) Пробуем использовать системные CA-пути по умолчанию
    ctx.set_default_verify_paths();
    // 2) Включаем верификацию сервера
    ctx.set_verify_mode(ssl::verify_peer);

    // 3) Попытка загрузить локальный CACert
    const std::string caPath = "cacert.pem"; // путь к файлу CACert в корне проекта

    std::ifstream fin(caPath);
    if (fin.good()) 
    {
        try 
        {
            ctx.load_verify_file(caPath);
            HttpLogger::log("HttpClient[HTTPS]", "Loaded CA bundle from " + caPath);
        }
        catch (const std::exception& e) 
        {
            HttpLogger::log("HttpClient[HTTPS]", std::string("Failed to load CA bundle from ") +
                            caPath + ": " + e.what());
        }
    }
    else 
    {
        HttpLogger::log("HttpClient[HTTPS]", "CA bundle not found at " + caPath +
                        "; using system/default paths");
    }
}

// Основная функция запроса
bool HttpClient::fetch(const std::string& url, std::string& content) 
{
    content.clear();

    HttpLogger::log("HttpClient", "Fetching URL: " + url);

    // Разбор URL
    std::string scheme, host, target, portStr;
    parseUrl(url, scheme, host, target, portStr);

    unsigned short port = 0;
    try 
    {
        port = static_cast<unsigned short>(std::stoi(portStr));
    }
    catch (...) 
    {
        port = (scheme == "https") ? 443 : 80;
    }

    try 
    {
        net::io_context ioc;

        if (scheme == "https") 
        {
            HttpLogger::log("HttpClient[HTTPS]", "Starting TLS connection to " + host + ":" + std::to_string(port));

            ssl::context ctx(ssl::context::tlsv12_client);
            // Настроим CA: системные пути + локальный CACert (если есть)
            configureCACertsIfAvailable(ctx);

            beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, std::to_string(port));

            beast::get_lowest_layer(stream).connect(results);
            
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {}

            // TLS handshake
            stream.handshake(ssl::stream_base::client);
            HttpLogger::log("HttpClient[HTTPS]", "TLS handshake completed with " + host);

            // HTTP GET
            http::request<http::string_body> req{ http::verb::get, target, 11 };
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            content = res.body();

            // Shutdown
            beast::error_code ec;
            stream.shutdown(ec);

            HttpLogger::log("HttpClient[HTTPS]", "Response: status=" + std::to_string(res.result_int()) +
                            ", body_size=" + std::to_string(content.size()));

            return res.result() == http::status::ok;
        }
        else 
        {
            HttpLogger::log("HttpClient[HTTP]", "Starting HTTP connection to " + host + ":" + std::to_string(port));

            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, std::to_string(port));

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

            beast::error_code ec;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            HttpLogger::log("HttpClient[HTTP]", "Response: status=" + std::to_string(res.result_int()) +
                            ", body_size=" + std::to_string(content.size()));

            return res.result() == http::status::ok;
        }
    }
    catch (const std::exception& e) 
    {
        HttpLogger::log("HttpClient", std::string("Exception in fetch: ") + e.what());
        return false;
    }
    catch (...) 
    {
        HttpLogger::log("HttpClient", "Unknown exception in fetch");
        return false;
    }
}