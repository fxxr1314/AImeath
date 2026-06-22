#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <sstream>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/json.hpp>

#include "gamemod.hpp"
#include "netconn.hpp"

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

// ---- Game session ----

struct GameSession {
    GameModuleCache cache;
    GameModule lib{};
    GamePtr game{nullptr, GameDeleter{nullptr}};
    std::string game_type;
    std::atomic<bool> paused{false};
};

static void handleGame(websocket::stream<beast::tcp_stream>& ws)
{
    GameSession session;
    beast::flat_buffer buf;

    while (true)
    {
        buf.clear();
        ws.read(buf);
        std::string msg = beast::buffers_to_string(buf.data());

        std::string action = jsonParseStr(msg, "action");
        if (action.empty()) action = jsonParseStr(msg, "type");

        auto send = [&](const std::string& data) {
            ws.write(asio::buffer(data));
        };

        if (action == "new_game") {
            if (session.game) { send(jsonError("game already running")); continue; }
            session.game_type = jsonParseStr(msg, "game");
            if (session.game_type.empty()) { send(jsonError("missing game")); continue; }
            session.lib = session.cache.load(session.game_type);
            if (!session.lib) { send(jsonError("failed to load " + session.game_type)); continue; }
            int w = jsonParseInt(msg, "width"); if (w <= 0) w = 20;
            int h = jsonParseInt(msg, "height"); if (h <= 0) h = 20;
            session.game = session.lib.create(w, h);
            session.paused = false;
            if (session.game)
                send(session.game->getState());
        }
        else if (action == "tick" || action == "input") {
            if (!session.game) { send(jsonError("no game")); continue; }
            if (session.paused) continue;
            int val = jsonParseInt(msg, "value");
            session.game->tick(val);
            send(session.game->getState());
        }
        else if (action == "pause") {
            session.paused = true;
            send(jsonOk());
        }
        else if (action == "resume") {
            session.paused = false;
            send(jsonOk());
        }
        else if (action == "end_game") {
            session.game.reset();
            send(jsonOk());
            break;
        }
        else {
            send(jsonError("unknown action: " + action));
        }
    }
}

// ---- Chat session (DeepSeek streaming) ----

static void sendJson(websocket::stream<beast::tcp_stream>& ws, const boost::json::object& obj)
{
    ws.write(asio::buffer(boost::json::serialize(obj)));
}

static void processSseData(websocket::stream<beast::tcp_stream>& ws,
                           std::string& buf,
                           std::string& full_response)
{
    size_t pos;
    while ((pos = buf.find("\n\n")) != std::string::npos)
    {
        std::string event = buf.substr(0, pos);
        buf.erase(0, pos + 2);

        std::istringstream ss(event);
        std::string line;
        while (std::getline(ss, line))
        {
            if (line.rfind("data: ", 0) != 0) continue;
            std::string data = line.substr(6);
            if (data == "[DONE]") continue;
            try
            {
                auto val = boost::json::parse(data);
                auto& choices = val.as_object().at("choices").as_array();
                if (choices.empty()) continue;
                auto& delta = choices[0].as_object().at("delta").as_object();
                if (!delta.contains("content")) continue;
                std::string content = delta.at("content").as_string().c_str();
                full_response += content;
                boost::json::object msg;
                msg["type"] = "delta";
                msg["text"] = content;
                sendJson(ws, msg);
            }
            catch (const std::exception&) {}
        }
    }
}

static std::string loadDeepSeekApiKey()
{
    std::ifstream f("config.json");
    if (!f.is_open())
    {
        std::cerr << "WARNING: config.json not found" << std::endl;
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try
    {
        auto val = boost::json::parse(content);
        auto& obj = val.as_object();
        if (obj.contains("deepseek_api_key"))
            return obj["deepseek_api_key"].as_string().c_str();
    }
    catch (const std::exception& e)
    {
        std::cerr << "WARNING: failed to parse config.json: " << e.what() << std::endl;
    }
    return "";
}

static std::string callDeepSeekStream(
    websocket::stream<beast::tcp_stream>& ws,
    const std::vector<boost::json::object>& history)
{
    boost::json::object start;
    start["type"] = "stream_start";
    sendJson(ws, start);

    std::string full_response;
    std::string leftover;
    auto on_chunk = [&](const std::string& chunk) {
        leftover += chunk;
        processSseData(ws, leftover, full_response);
    };

    try
    {
        boost::json::array msgs;
        for (const auto& m : history) msgs.push_back(m);
        boost::json::object body;
        body["model"] = "deepseek-v4-flash";
        body["messages"] = msgs;
        body["stream"] = true;
        body["thinking"] = {{"type", "disabled"}};

        std::string apiKey = loadDeepSeekApiKey();
        if (apiKey.empty())
        {
            boost::json::object end;
            end["type"] = "stream_end";
            end["msg"]  = "missing deepseek_api_key in config.json";
            try { sendJson(ws, end); } catch (...) {}
            return full_response;
        }

        httpsPostStream(
            "api.deepseek.com", "443", "/chat/completions",
            boost::json::serialize(body), "application/json",
            "Bearer " + apiKey,
            on_chunk
        );

        processSseData(ws, leftover, full_response);
    }
    catch (const std::exception& e)
    {
        std::string what = e.what();
        if (what.find("eof") == std::string::npos &&
            what.find("end of stream") == std::string::npos)
        {
            boost::json::object end;
            end["type"] = "stream_end";
            end["msg"]  = what;
            try { sendJson(ws, end); } catch (...) {}
        }
        return full_response;
    }

    boost::json::object end;
    end["type"] = "stream_end";
    try { sendJson(ws, end); } catch (...) {}
    return full_response;
}

static void handleCommand(websocket::stream<beast::tcp_stream>& ws,
                          const std::string& text)
{
    std::string s = text.substr(1); // remove leading /

    // Trim trailing whitespace so "/图片 " matches too
    size_t end = s.find_last_not_of(" \t");
    if (end != std::string::npos) s = s.substr(0, end + 1);

    // Split into cmd and arg at first space/tab
    size_t pos = s.find_first_of(" \t");
    std::string cmd = (pos == std::string::npos) ? s : s.substr(0, pos);
    std::string arg;
    if (pos != std::string::npos) {
        arg = s.substr(pos + 1);
        size_t first = arg.find_first_not_of(" \t");
        if (first != std::string::npos) arg = arg.substr(first);
    }

    boost::json::object embed;
    embed["type"] = "embed";

    if (s == "图片")
    {
        embed["kind"] = "image";
        embed["url"]  = "/res/C220748556D18ADBC61177B1A5A8151D.png";
        embed["title"]= "照片";
    }
    else if (s == "视频")
    {
        embed["kind"] = "video";
        embed["url"]  = "/res/result.mp4";
        embed["title"]= "视频";
    }
    else if (s == "音乐")
    {
        embed["kind"] = "audio";
        embed["url"]  = "/res/超级敏感.wav";
        embed["title"]= "超级敏感";
    }
    else if (cmd == "游戏" || s.rfind("游戏", 0) == 0)
    {
        // /游戏, /游戏 snake (space-separated), /游戏snake (inline)
        std::string game;
        if (cmd == "游戏")
            game = arg;
        else if (s.size() > 6)
            game = s.substr(6);       // "游戏" = 6 UTF-8 bytes
        if (game.empty()) game = "snake";
        embed["kind"] = "game";
        embed["name"] = game;
        embed["url"]  = "/" + game;
        embed["title"]= "游戏：" + game;
    }
    else
    {
        embed["kind"] = "text";
        embed["text"] = "未知命令: /" + cmd;
    }

    sendJson(ws, embed);
}

static void handleChat(websocket::stream<beast::tcp_stream>& ws)
{
    std::vector<boost::json::object> history;
    beast::flat_buffer buf;

    try
    {
        while (true)
        {
            buf.clear();
            ws.read(buf);
            std::string msg = beast::buffers_to_string(buf.data());
            std::string text = jsonParseStr(msg, "text");
            if (text.empty()) continue;

            // Commands: /图片 /视频 /音乐 /游戏xxx
            if (!text.empty() && text[0] == '/')
            {
                handleCommand(ws, text);
                continue;
            }

            boost::json::object user_msg;
            user_msg["role"] = "user";
            user_msg["content"] = text;
            history.push_back(user_msg);

            std::string reply = callDeepSeekStream(ws, history);
            if (!reply.empty())
            {
                boost::json::object assistant_msg;
                assistant_msg["role"] = "assistant";
                assistant_msg["content"] = reply;
                history.push_back(assistant_msg);
            }
        }
    }
    catch (const boost::system::system_error&) {}
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

        if (req.target() == "/chat")
            handleChat(ws);
        else
            handleGame(ws);
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
