#include <gtest/gtest.h>
#include "llm_client.hpp"

#include <boost/asio.hpp>

TEST(LlmClientTest, CreateDestroy)
{
    boost::asio::io_context io;
    {
        auto client = std::make_shared<LlmClient>(io);
        EXPECT_NE(client, nullptr);
    }
}

TEST(LlmClientTest, CancelWithoutStart)
{
    boost::asio::io_context io;
    {
        auto client = std::make_shared<LlmClient>(io);
        client->cancel();
    }
}

TEST(LlmClientTest, CancelAfterStartBeforeResolve)
{
    boost::asio::io_context io;
    auto client = std::make_shared<LlmClient>(io);
    client->start("api.deepseek.com", "443", "{}", "",
        [](LlmEvent) {}, [](std::string, std::string) {});
    client->cancel();
    io.run_for(std::chrono::milliseconds(100));
}

TEST(LlmClientTest, StartWithInvalidHost)
{
    boost::asio::io_context io;
    bool event_called = false;
    auto client = std::make_shared<LlmClient>(io);
    client->start("nonexistent.invalid", "443", "{}", "",
        [&](LlmEvent ev) {
            if (ev.type == "stream_end") event_called = true;
        },
        [](std::string, std::string) {},
        std::chrono::seconds(2));
    io.run_for(std::chrono::seconds(5));
    EXPECT_TRUE(event_called);
}
