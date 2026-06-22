#include "chat.hpp"

Chat::Chat(MeowFn fn)
    : m_fn(std::move(fn))
{
}

std::string Chat::process(const std::string& input)
{
    return m_fn(input);
}

extern "C"
{

void* chat_new(char* (*meow)(const char*))
{
    return new Chat([meow](const std::string& s) -> std::string {
        char* c_result = meow(s.c_str());
        std::string result(c_result ? c_result : "");
        std::free(c_result);
        return result;
    });
}

void chat_free(void* chat)
{
    delete static_cast<Chat*>(chat);
}

char* chat_process(void* chat, const char* input)
{
    std::string result = static_cast<Chat*>(chat)->process(input);
    char* buf = static_cast<char*>(std::malloc(result.size() + 1));
    if (buf)
        std::memcpy(buf, result.data(), result.size() + 1);
    return buf;
}

}
