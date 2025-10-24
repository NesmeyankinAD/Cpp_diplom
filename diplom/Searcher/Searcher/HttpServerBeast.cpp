#include "HttpServerBeast.h"
#include "HtmlRenderer.h"
#include "SearcherEngine.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>

#include <string>
#include <sstream>


std::string urlDecode(const std::string& in) 
{
    std::string out;

    for (size_t i = 0; i < in.size(); ++i) 
    {
        char c = in[i];
        if (c == '+') {out += ' ';}

        else if (c == '%' && i + 2 < in.size()) 
        {
            std::string hex = in.substr(i + 1, 2);
            unsigned int val = 0;
            std::istringstream iss(hex);

            if (iss >> std::hex >> val) 
            {
                out += static_cast<char>(val);
                i += 2;
            }
            else {out += c;}
        }
        else {out += c;}
    }
    return out;
}

std::string parseForm(const std::string& body, const std::string& key = "q") 
{
    std::string needle = key + "=";
    auto pos = body.find(needle);

    if (pos == std::string::npos) return "";

    auto valStart = pos + needle.size();
    auto end = body.find('&', valStart);

    std::string val = body.substr(valStart, end == std::string::npos ? std::string::npos : end - valStart);
    
    return urlDecode(val);
}

std::string extractQueryParam(const std::string& target, const std::string& key = "q") 
{
    // target может быть "/search?q=..." или "/?q=..."

    auto qpos = target.find('?');

    if (qpos == std::string::npos) return "";

    auto query = target.substr(qpos + 1);
    auto keypos = query.find(key + "=");

    if (keypos == std::string::npos) return "";

    auto valStart = keypos + key.length() + 1;
    auto end = query.find('&', valStart);

    std::string val = query.substr(valStart, end == std::string::npos ? std::string::npos : end - valStart);
    
    return urlDecode(val);
}


HttpServerBeast::HttpServerBeast(boost::asio::io_context& ioc,
                                 unsigned short port,
                                 SearcherEngine* engine,
                                 HtmlRenderer* renderer)
    : ioc_(ioc),
    port_(port),
    acceptor_(ioc),
    engine_(engine),
    renderer_(renderer)
{
    boost::system::error_code ec;
    using tcp = boost::asio::ip::tcp;
    tcp::endpoint endpoint(tcp::v4(), port_);
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) return;
    acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);
    if (ec) return;
    acceptor_.bind(endpoint, ec);
    if (ec) return;
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
}

void HttpServerBeast::run()
{
    using tcp = boost::asio::ip::tcp;

    for (;;) 
    {
        boost::system::error_code ec;
        tcp::socket socket(ioc_);
        acceptor_.accept(socket, ec);
        if (ec) { continue; }

        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;
        ec.clear();
        boost::beast::http::read(socket, buffer, req, ec);
        if (ec) 
        {
            socket.close();
            continue;
        }

        std::string target(req.target().data(), req.target().size());
        std::string html;

        if (req.method() == boost::beast::http::verb::get) 
        {
            if (target == "/" || target == "/index.html") 
            {
                html = renderer_->renderIndex();
            }

            else if (target.find("/search") == 0) 
            {
                std::string q = extractQueryParam(target, "q");
                auto results = engine_->search(q);
                html = renderer_->render(results, q);
            }
            else 
            {
                html = renderer_->renderIndex();
            }
        }
        else if (req.method() == boost::beast::http::verb::post && target == "/search") 
        {
            std::string q = parseForm(req.body(), "q");
            auto results = engine_->search(q);
            html = renderer_->render(results, q);
        }
        else 
        {
            boost::beast::http::response<boost::beast::http::string_body> res
                {boost::beast::http::status::not_found, 11 };

            res.set(boost::beast::http::field::content_type, "text/html");
            res.body() = "<h1>Not Found</h1>";
            res.prepare_payload();
            boost::beast::http::write(socket, res, ec);
            socket.shutdown(tcp::socket::shutdown_send, ec);
            continue;
        }

        boost::beast::http::response<boost::beast::http::string_body> res
            {boost::beast::http::status::ok, 11 };

        res.set(boost::beast::http::field::content_type, "text/html");
        res.body() = html;
        res.prepare_payload();
        boost::beast::http::write(socket, res, ec);
        socket.shutdown(tcp::socket::shutdown_send, ec);
    }
}