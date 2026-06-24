/**
 * gameserver — 统一 WebSocket 服务端
 *
 * 监听 3001 端口，接收 WebSocket 连接。根据客户端首条消息的 JSON
 * 内容自动路由到对应的 app 动态库（.so）：
 *   - 包含 "app"   字段 → 使用指定 app 名称
 *   - 包含 "text"  字段 → 路由到 chat
 *   - 包含 "game"  字段 → 路由到对应游戏
 *   - 其它            → 路由到 snake
 *
 * 每个连接由 ThreadPool 调度执行，通过 C ABI (app_create /
 * app_process / app_is_done) 与 app 通信。
 */

#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <csignal>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>

#include "app_mod.hpp"
#include "threadmgr.hpp"
#include "logger.hpp"
#include "wsutil.hpp"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// ---- 常量 ----

constexpr int PORT = 3001;
constexpr int THREAD_COUNT = 4;

namespace key {
    constexpr auto APP  = "app";
    constexpr auto TEXT = "text";
    constexpr auto GAME = "game";
}

namespace appname {
    constexpr auto CHAT  = "chat";
    constexpr auto SNAKE = "snake";
}

// ============================================================
//  App 消息循环
// ============================================================

static void handleApp(websocket::stream<beast::tcp_stream>& ws,
                      const std::string& first_msg,
                      AppModule& mod, void* app)
{
    // 处理首条消息（创建 app 时传入的 config / 命令）
    char* out = mod.app_process(app, first_msg.c_str());
    if (out) {
        try {
            auto arr = boost::json::parse(out).as_array();
            for (auto& item : arr)
                ws.write(asio::buffer(boost::json::serialize(item)));
        } catch (...) {}
        mod.app_free_string(out);
    }

    // 循环处理后续消息
    beast::flat_buffer buf;
    while (!mod.app_is_done(app)) {
        buf.clear();
        ws.read(buf);
        std::string msg = beast::buffers_to_string(buf.data());
        out = mod.app_process(app, msg.c_str());
        if (out) {
            try {
                auto arr = boost::json::parse(out).as_array();
                for (auto& item : arr)
                    ws.write(asio::buffer(boost::json::serialize(item)));
            } catch (...) {}
            mod.app_free_string(out);
        }
    }
}

// ============================================================
//  客户端连接处理
// ============================================================

static void handleClient(tcp::socket socket, Logger& logger)
{
    try {
        beast::tcp_stream stream(std::move(socket));
        beast::flat_buffer buf;
        http::request<http::string_body> req;
        http::read(stream, buf, req);

        websocket::stream<beast::tcp_stream> ws(std::move(stream));
        ws.accept(req);

        // 读取首条消息用于路由
        buf.clear();
        ws.read(buf);
        std::string first_msg = beast::buffers_to_string(buf.data());

        // 根据 JSON 字段路由到对应 app
        std::string app_name;
        {
            auto val = boost::json::parse(first_msg);
            if (!val.is_object()) {
                ws.write(asio::buffer(jsonError("invalid first message")));
                return;
            }

            std::string text = jsonParseStr(val, key::APP);
            if (!text.empty()) {
                app_name = std::move(text);
            } else {
                text = jsonParseStr(val, key::TEXT);
                if (!text.empty()) {
                    app_name = appname::CHAT;
                } else {
                    text = jsonParseStr(val, key::GAME);
                    if (!text.empty())
                        app_name = std::move(text);
                    else
                        app_name = appname::SNAKE;
                }
            }
        }

        logger.info() << "Routing to app: " << app_name;

        // 加载 app 动态库（带缓存）
        static AppModuleCache cache;
        AppModule mod = cache.load(app_name);
        if (!mod) {
            ws.write(asio::buffer(jsonError("failed to load " + app_name)));
            return;
        }

        AppPtr app = mod.create(first_msg);
        if (!app) {
            ws.write(asio::buffer(jsonError("failed to create " + app_name + " instance")));
            return;
        }

        handleApp(ws, first_msg, mod, app.get());
    }
    catch (const boost::system::system_error&) {
        // 连接异常断开，静默处理
    }
}

// ============================================================
//  主函数
// ============================================================

int main()
{
    Logger logger(std::cout, Logger::INFO);

    // 有界的线程池，替代无限制的 std::thread().detach()
    ThreadPool pool(THREAD_COUNT);
    auto& io = pool.io_context();

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), PORT));

    // 独立的 io_context 处理信号，避免死锁
    asio::io_context sig_io;
    asio::signal_set signals(sig_io, SIGINT, SIGTERM);
    signals.async_wait([&acceptor, &logger](auto ec, auto sig) {
        if (!ec) {
            logger.info() << "Signal " << sig << " received, shutting down...";
            acceptor.close();  // 解除主线程 accept 阻塞
        }
    });
    std::thread sig_thread([&sig_io] { sig_io.run(); });

    logger.info() << "Game server listening on port " << PORT;

    // 主线程阻塞在 accept，信号到来时 acceptor.close() 使 accept 抛异常退出
    while (true) {
        beast::error_code ec;
        tcp::socket socket(io);
        acceptor.accept(socket, ec);
        if (ec) {
            if (ec == asio::error::operation_aborted) break;
            logger.error() << "Accept error: " << ec.message();
            break;
        }
        auto sock = std::make_shared<tcp::socket>(std::move(socket));
        pool.submit([sock, &logger]() {
            handleClient(std::move(*sock), logger);
        });
    }

    sig_io.stop();
    sig_thread.join();

    logger.info() << "Waiting for active connections...";
    pool.wait_all();
    pool.shutdown();

    logger.info() << "Server stopped.";
    return 0;
}
