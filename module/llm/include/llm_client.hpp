#pragma once

#include <string>
#include <functional>
#include <memory>
#include <chrono>

namespace boost::asio {
class io_context;
}

struct LlmEvent {
    std::string type;
    std::string text;
    std::string msg;
};

using LlmEventFn = std::function<void(LlmEvent)>;
using LlmDoneFn = std::function<void(std::string full_response, std::string full_reasoning)>;

class LlmClient
{
public:
    explicit LlmClient(boost::asio::io_context& io);
    ~LlmClient();

    void start(const std::string& host, const std::string& port,
               const std::string& body, const std::string& auth,
               LlmEventFn on_event, LlmDoneFn on_done,
               std::chrono::seconds timeout = std::chrono::seconds(120));

    void cancel();

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
