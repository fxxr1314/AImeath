#include "agent_server.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>

#include <boost/asio.hpp>
#include <boost/json.hpp>

#include "llm_client.hpp"
#include "llm_utils.hpp"
#include "config.hpp"

namespace asio = boost::asio;

namespace agent {

#define AGENT_LOG(level, msg) \
    do { \
        auto now = std::chrono::system_clock::now(); \
        auto t = std::chrono::system_clock::to_time_t(now); \
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( \
            now.time_since_epoch()) % 1000; \
        char buf[32]; \
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t)); \
        std::cerr << "[" << buf << "." << ms.count() << "] [agent] " << level << " " << msg << std::endl; \
    } while(0)

static boost::json::object buildSystemMsg()
{
    boost::json::object msg;
    msg["role"] = "system";
    msg["content"] =
        "You are an AI Agent assistant. You can help users by opening and controlling applications.\n"
        "When a user asks you to do something, use the available tools to execute actions.\n"
        "After each tool execution, briefly explain what you did in Chinese.\n"
        "Available tools: open_app (open an application), control_app (send commands to an app), "
        "close_app (close an application), list_apps (list available apps).\n"
        "Keep responses concise and friendly.";
    return msg;
}

boost::json::array AgentServer::buildTools()
{
    boost::json::array tools;

    {
        boost::json::object t;
        t["type"] = "function";
        boost::json::object f;
        f["name"] = "open_app";
        f["description"] = "打开一个应用程序窗口. Use this when the user asks to open or launch an app.";
        boost::json::object params;
        params["type"] = "object";
        boost::json::object props;
        boost::json::object appProp;
        appProp["type"] = "string";
        appProp["description"] = "应用名称, 可选: snake, gomoku, pacman, go, chat, terminal, filemanager";
        props["app"] = appProp;
        boost::json::object wProp;
        wProp["type"] = "integer";
        wProp["description"] = "棋盘宽度 (游戏类应用, default 20)";
        props["width"] = wProp;
        boost::json::object hProp;
        hProp["type"] = "integer";
        hProp["description"] = "棋盘高度 (游戏类应用, default 20)";
        props["height"] = hProp;
        params["properties"] = props;
        boost::json::array required;
        required.push_back(boost::json::string("app"));
        params["required"] = required;
        f["parameters"] = params;
        t["function"] = f;
        tools.push_back(t);
    }

    {
        boost::json::object t;
        t["type"] = "function";
        boost::json::object f;
        f["name"] = "control_app";
        f["description"] =
            "向已打开的应用发送操作指令. Use this to control a running app, "
            "e.g., move in a game. Direction values: 0=up, 1=down, 2=left, 3=right. "
            "For go: -1=pass, -2=resign.";
        boost::json::object params;
        params["type"] = "object";
        boost::json::object props;
        boost::json::object appProp;
        appProp["type"] = "string";
        appProp["description"] = "目标应用名称";
        props["app"] = appProp;
        boost::json::object valProp;
        valProp["type"] = "integer";
        valProp["description"] = "操作值 (游戏方向等)";
        props["value"] = valProp;
        params["properties"] = props;
        boost::json::array required;
        required.push_back(boost::json::string("app"));
        required.push_back(boost::json::string("value"));
        params["required"] = required;
        f["parameters"] = params;
        t["function"] = f;
        tools.push_back(t);
    }

    {
        boost::json::object t;
        t["type"] = "function";
        boost::json::object f;
        f["name"] = "close_app";
        f["description"] = "关闭一个已打开的应用窗口.";
        boost::json::object params;
        params["type"] = "object";
        boost::json::object props;
        boost::json::object appProp;
        appProp["type"] = "string";
        appProp["description"] = "要关闭的应用名称";
        props["app"] = appProp;
        params["properties"] = props;
        boost::json::array required;
        required.push_back(boost::json::string("app"));
        params["required"] = required;
        f["parameters"] = params;
        t["function"] = f;
        tools.push_back(t);
    }

    return tools;
}

void AgentServer::registerBuiltinTools()
{
    tools_["open_app"] = {"open_app", "打开应用", {}, nullptr};
    tools_["control_app"] = {"control_app", "操控应用", {}, nullptr};
    tools_["close_app"] = {"close_app", "关闭应用", {}, nullptr};
}

AgentServer::AgentServer()
{
    registerBuiltinTools();
    boost::json::object sys = buildSystemMsg();
    history_.push_back(sys);
}

void AgentServer::setOutput(app_output_fn cb, void* udata)
{
    std::lock_guard<std::mutex> lock(mtx_);
    outputCb_ = cb;
    outputUdata_ = udata;
}

void AgentServer::setIoContext(void* ioCtx)
{
    ioCtxPtr_ = ioCtx;
}

void AgentServer::onInput(const std::string& json)
{
    try {
        auto val = boost::json::parse(json);
        if (!val.is_object()) return;
        auto& obj = val.as_object();

        auto actionIt = obj.find("action");
        if (actionIt != obj.end() && actionIt->value().is_string())
        {
            auto action = actionIt->value().as_string();
            if (action == "stop") {
                AGENT_LOG("[in]", "stop request");
                stop();
                boost::json::object end;
                end["type"] = "stream_end";
                end["msg"] = "stopped";
                pushOutput(std::move(end));
                return;
            }
        }

        auto textIt = obj.find("text");
        if (textIt != obj.end() && textIt->value().is_string())
        {
            std::string text(textIt->value().as_string());
            AGENT_LOG("[in]", "text: \"" << text.substr(0, 30)
                 << (text.size() > 30 ? "..." : "") << "\""
                 << " (streaming=" << streaming_ << ")");
            if (streaming_) {
                inputQueue_.push(text);
                AGENT_LOG("[q]", "queued (size=" << inputQueue_.size() << ")");
            } else {
                streaming_ = true;
                ++round_;
                handleUserMessage(text);
            }
        }
    } catch (...) {}
}

void AgentServer::handleUserMessage(const std::string& text)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        boost::json::object um;
        um["role"] = "user";
        um["content"] = text;
        history_.push_back(std::move(um));
    }

    std::vector<boost::json::object> historyCopy;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        historyCopy = history_;
    }

    boost::json::array msgs;
    for (auto& m : historyCopy) msgs.push_back(m);

    std::string body = llm::build_chat_body(msgs);
    boost::json::array tools = buildTools();

    boost::json::value parsed = boost::json::parse(body);
    if (parsed.is_object()) {
        parsed.as_object()["tools"] = tools;
        parsed.as_object()["tool_choice"] = boost::json::string("auto");
        body = boost::json::serialize(parsed);
    }

    std::string apiKey = Config::instance().deepSeekApiKey();
    if (apiKey.empty()) {
        boost::json::object end;
        end["type"] = "stream_end";
        end["msg"] = "missing deepseek_api_key in config.json";
        pushOutput(std::move(end));
        return;
    }

    asio::io_context* io = static_cast<asio::io_context*>(ioCtxPtr_);
    if (!io) {
        auto self = shared_from_this();
        std::thread t([self, body = std::move(body), apiKey]() {
            asio::io_context io;
            auto client = std::make_shared<::LlmClient>(io);
            std::string fullResponse;

            client->start("api.deepseek.com", "443", body, "Bearer " + apiKey,
                [&](::LlmEvent ev) {
                    if (self->cancelled_) { client->cancel(); return; }
                    boost::json::object obj;
                    obj["type"] = ev.type;
                    if (ev.type == "delta" || ev.type == "reasoning")
                        obj["text"] = std::move(ev.text);
                    if (!ev.msg.empty())
                        obj["msg"] = std::move(ev.msg);
                    if (!ev.tool_calls.empty()) {
                        boost::json::array tcArr;
                        for (auto& tc : ev.tool_calls) {
                            boost::json::object tcObj;
                            tcObj["id"] = tc.id;
                            tcObj["function_name"] = tc.function_name;
                            tcObj["function_arguments"] = tc.function_arguments;
                            tcArr.push_back(std::move(tcObj));
                        }
                        obj["tool_calls"] = std::move(tcArr);
                    }
                    self->pushOutput(std::move(obj));
                },
                [&](std::string response, std::string, int, int) {
                    fullResponse = std::move(response);
                });

            io.run();

            if (!self->cancelled_) {
                if (!fullResponse.empty()) {
                    boost::json::object am;
                    am["role"] = "assistant";
                    am["content"] = fullResponse;
                    std::lock_guard<std::mutex> lock(self->mtx_);
                    self->history_.push_back(std::move(am));
                }
                self->streaming_ = false;
                self->processNextInQueue();
            }
        });
        t.detach();
    } else {
        std::weak_ptr<AgentServer> weakSelf = shared_from_this();
        auto pushEvent = [weakSelf](::LlmEvent ev) {
            auto self = weakSelf.lock();
            if (!self) return;
            boost::json::object obj;
            obj["type"] = ev.type;
            if (ev.type == "delta" || ev.type == "reasoning")
                obj["text"] = std::move(ev.text);
            if (!ev.msg.empty())
                obj["msg"] = std::move(ev.msg);
            if (!ev.tool_calls.empty()) {
                boost::json::array tcArr;
                for (auto& tc : ev.tool_calls) {
                    boost::json::object tcObj;
                    tcObj["id"] = tc.id;
                    tcObj["function_name"] = tc.function_name;
                    tcObj["function_arguments"] = tc.function_arguments;
                    tcArr.push_back(std::move(tcObj));
                }
                obj["tool_calls"] = std::move(tcArr);
            }
            self->pushOutput(std::move(obj));
        };

        std::weak_ptr<AgentServer> weakSelf2 = shared_from_this();
        auto onDone = [weakSelf2](std::string response, std::string, int, int) {
            auto self = weakSelf2.lock();
            if (!self || self->cancelled_) return;
            if (!response.empty()) {
                boost::json::object am;
                am["role"] = "assistant";
                am["content"] = std::move(response);
                std::lock_guard<std::mutex> lock(self->mtx_);
                self->history_.push_back(std::move(am));
            }
            self->streaming_ = false;
            self->processNextInQueue();
        };

        auto stream = std::make_shared<::LlmClient>(*io);
        currentStream_ = stream;
        stream->start("api.deepseek.com", "443",
            body, "Bearer " + apiKey,
            std::move(pushEvent), std::move(onDone));
    }

    boost::json::object start;
    start["type"] = "stream_start";
    pushOutput(std::move(start));
}

void AgentServer::processNextInQueue()
{
    if (cancelled_) return;
    std::string next;
    if (!inputQueue_.try_pop(next)) {
        AGENT_LOG("[q]", "empty, idle (round " << round_ << ")");
        return;
    }
    AGENT_LOG("[q]", "dequeue (round " << round_ << ", " << inputQueue_.size() << " left)");
    streaming_ = true;
    ++round_;
    handleUserMessage(next);
}

void AgentServer::pushOutput(boost::json::value val)
{
    app_output_fn cb = nullptr;
    void* udata = nullptr;
    std::string s;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        cb = outputCb_;
        udata = outputUdata_;
        if (cb) s = boost::json::serialize(val);
    }
    if (cb) cb(udata, s.c_str());
}

bool AgentServer::openApp(const std::string& name, const std::string& paramsJson)
{
    boost::json::object out;
    out["type"] = "agent";
    out["action"] = "open_app";
    out["app"] = name;
    try {
        out["params"] = boost::json::parse(paramsJson);
    } catch (...) {
        out["params"] = boost::json::object{};
    }
    pushOutput(std::move(out));
    return true;
}

bool AgentServer::controlApp(const std::string& name, const std::string& commandJson)
{
    boost::json::object out;
    out["type"] = "agent";
    out["action"] = "control_app";
    out["app"] = name;
    try {
        out["command"] = boost::json::parse(commandJson);
    } catch (...) {
        out["command"] = boost::json::object{};
    }
    pushOutput(std::move(out));
    return true;
}

bool AgentServer::closeApp(const std::string& name)
{
    boost::json::object out;
    out["type"] = "agent";
    out["action"] = "close_app";
    out["app"] = name;
    pushOutput(std::move(out));
    return true;
}

void AgentServer::stop()
{
    cancelled_ = true;
    if (currentStream_) currentStream_->cancel();
    streaming_ = false;
    inputQueue_.clear();
}

bool AgentServer::isDone() const
{
    return done_;
}

void AgentServer::destroy()
{
    cancelled_ = true;
    stop();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        outputCb_ = nullptr;
        outputUdata_ = nullptr;
    }
    currentStream_.reset();
    selfHolder_.reset();
}

std::string AgentServer::process(const std::string& json)
{
    boost::json::array outputs;
    try {
        auto val = boost::json::parse(json);
        if (!val.is_object()) goto done;
        auto& obj = val.as_object();

        auto textIt = obj.find("text");
        if (textIt != obj.end() && textIt->value().is_string())
        {
            std::string text(textIt->value().as_string());
            if (text[0] == '/') {
                boost::json::object embed;
                embed["type"] = "embed";
                embed["kind"] = "text";
                embed["text"] = "命令: " + text;
                outputs.push_back(embed);
            } else {
                onInput(json);
                boost::json::object start;
                start["type"] = "stream_start";
                outputs.push_back(std::move(start));
                boost::json::object msg;
                msg["type"] = "delta";
                msg["text"] = "Agent is processing your request...";
                outputs.push_back(std::move(msg));
            }
        }
    } catch (...) {}

done:
    return boost::json::serialize(outputs);
}

// ============================================================
//  C ABI
// ============================================================

} // namespace agent

extern "C" {

void* app_create(const char* configJson)
{
    (void)configJson;
    auto ptr = std::make_shared<agent::AgentServer>();
    ptr->selfHolder_ = ptr;
    return ptr.get();
}

void app_destroy(void* p)
{
    static_cast<agent::AgentServer*>(p)->destroy();
}

void app_set_output(void* p, app_output_fn cb, void* userdata)
{
    static_cast<agent::AgentServer*>(p)->setOutput(cb, userdata);
}

void app_set_io_context(void* p, void* ioCtx)
{
    static_cast<agent::AgentServer*>(p)->setIoContext(ioCtx);
}

void app_on_input(void* p, const char* inputJson)
{
    static_cast<agent::AgentServer*>(p)->onInput(inputJson);
}

char* app_process(void* p, const char* inputJson)
{
    std::string result = static_cast<agent::AgentServer*>(p)->process(inputJson);
    char* buf = static_cast<char*>(std::malloc(result.size() + 1));
    if (buf) std::memcpy(buf, result.data(), result.size() + 1);
    return buf;
}

void app_free_string(char* str)
{
    std::free(str);
}

int app_is_done(void* p)
{
    return static_cast<agent::AgentServer*>(p)->isDone() ? 1 : 0;
}

} // extern "C"
