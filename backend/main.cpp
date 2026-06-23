#include <iostream>
#include <string>
#include <thread>
#include <memory>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>

#include "app_mod.hpp"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// ---- Generic app handler (works with any app .so) ----

static void handleApp(websocket::stream<beast::tcp_stream>& ws,
                      const std::string& first_msg,
                      AppModule& mod, void* app)
{
    // Process the first message
    char* out = mod.app_process(app, first_msg.c_str());
    if (out)
    {
        // Parse as JSON array and send each element
        try {
            auto arr = boost::json::parse(out).as_array();
            for (auto& item : arr)
                ws.write(asio::buffer(boost::json::serialize(item)));
        } catch (...) {}
        mod.app_free_string(out);
    }

    // Loop for subsequent messages
    beast::flat_buffer buf;
    while (!mod.app_is_done(app))
    {
        buf.clear();
        ws.read(buf);
        std::string msg = beast::buffers_to_string(buf.data());

        out = mod.app_process(app, msg.c_str());
        if (out)
        {
            try {
                auto arr = boost::json::parse(out).as_array();
                for (auto& item : arr)
                    ws.write(asio::buffer(boost::json::serialize(item)));
            } catch (...) {}
            mod.app_free_string(out);
        }
    }
}

// ---- Router ----

static void handleClient(tcp::socket socket)
{
    try
    {
        beast::tcp_stream stream(std::move(socket));
        beast::flat_buffer buf;
        http::request<http::string_body> req;
        http::read(stream, buf, req);

        websocket::stream<beast::tcp_stream> ws(std::move(stream));
        ws.accept(req);

        // Read the first message to determine which app to load
        buf.clear();
        ws.read(buf);
        std::string first_msg = beast::buffers_to_string(buf.data());

        // Determine app name from the message
        std::string app_name;
        try {
            auto val = boost::json::parse(first_msg);
            if (!val.is_object()) { ws.write(asio::buffer("[]")); return; }
            auto& obj = val.as_object();

            auto it = obj.find("app");
            if (it != obj.end() && it->value().is_string()) {
                app_name = std::string(it->value().as_string());
            } else if ((it = obj.find("text")) != obj.end() && it->value().is_string()) {
                app_name = "chat";
            } else if ((it = obj.find("game")) != obj.end() && it->value().is_string()) {
                app_name = std::string(it->value().as_string());
            } else {
                app_name = "snake"; // fallback
            }
        } catch (...) {
            ws.write(asio::buffer("[]"));
            return;
        }

        // Load the app module and create instance
        static AppModuleCache cache;
        AppModule mod = cache.load(app_name);
        if (!mod)
        {
            boost::json::object err;
            err["type"] = "error";
            err["msg"]  = "failed to load " + app_name;
            ws.write(asio::buffer(boost::json::serialize(err)));
            return;
        }

        AppPtr app = mod.create(first_msg);
        if (!app)
        {
            boost::json::object err;
            err["type"] = "error";
            err["msg"]  = "failed to create " + app_name + " instance";
            ws.write(asio::buffer(boost::json::serialize(err)));
            return;
        }

        handleApp(ws, first_msg, mod, app.get());
    }
    catch (const boost::system::system_error&)
    {
    }
}

// ---- Main ----

int main()
{
    try
    {
        asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 3001));
        std::cout << "Game server listening on port 3001" << std::endl;

        while (true)
        {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::thread(handleClient, std::move(socket)).detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
