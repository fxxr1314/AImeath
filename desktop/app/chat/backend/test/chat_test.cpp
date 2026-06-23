#include <gtest/gtest.h>
#include "chat.hpp"

static std::string meow(const std::string& text)
{
    return text + "\xe5\x96\xb5";
}

static char* meow_c(const char* text)
{
    std::string s(text);
    s += "\xe5\x96\xb5";
    char* buf = static_cast<char*>(std::malloc(s.size() + 1));
    if (buf)
        std::memcpy(buf, s.data(), s.size() + 1);
    return buf;
}

TEST(ChatTest, ProcessAppendsMeow)
{
    Chat chat(meow);
    EXPECT_EQ(chat.process(""), "\xe5\x96\xb5");
    EXPECT_EQ(chat.process("hello"), "hello\xe5\x96\xb5");
    EXPECT_EQ(chat.process("你好"), "你好\xe5\x96\xb5");
}

TEST(ChatTest, ProcessMultipleCalls)
{
    Chat chat(meow);
    EXPECT_EQ(chat.process("a"), "a\xe5\x96\xb5");
    EXPECT_EQ(chat.process("b"), "b\xe5\x96\xb5");
    EXPECT_EQ(chat.process("c"), "c\xe5\x96\xb5");
}

TEST(ChatTest, ProcessWithChineseInput)
{
    Chat chat(meow);
    EXPECT_EQ(chat.process("喵"), "喵\xe5\x96\xb5");
    EXPECT_EQ(chat.process("今天天气不错"), "今天天气不错\xe5\x96\xb5");
}

TEST(ChatTest, CapiNewFree)
{
    void* chat = chat_new(meow_c);
    ASSERT_NE(chat, nullptr);
    chat_free(chat);
}

TEST(ChatTest, CapiProcess)
{
    void* chat = chat_new(meow_c);
    ASSERT_NE(chat, nullptr);

    char* r1 = chat_process(chat, "");
    EXPECT_STREQ(r1, "\xe5\x96\xb5");
    std::free(r1);

    char* r2 = chat_process(chat, "hello");
    EXPECT_STREQ(r2, "hello\xe5\x96\xb5");
    std::free(r2);

    char* r3 = chat_process(chat, "你好");
    EXPECT_STREQ(r3, "你好\xe5\x96\xb5");
    std::free(r3);

    chat_free(chat);
}

TEST(ChatTest, CapiMultipleInstances)
{
    void* c1 = chat_new(meow_c);
    void* c2 = chat_new(meow_c);
    ASSERT_NE(c1, nullptr);
    ASSERT_NE(c2, nullptr);

    char* r1 = chat_process(c1, "foo");
    char* r2 = chat_process(c2, "bar");
    EXPECT_STREQ(r1, "foo\xe5\x96\xb5");
    EXPECT_STREQ(r2, "bar\xe5\x96\xb5");
    std::free(r1);
    std::free(r2);

    chat_free(c1);
    chat_free(c2);
}
