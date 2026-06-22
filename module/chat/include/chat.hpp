#pragma once

#include <string>
#include <functional>
#include <cstdlib>
#include <cstring>

class Chat
{
public:
    using MeowFn = std::function<std::string(const std::string&)>;

    explicit Chat(MeowFn fn);
    std::string process(const std::string& input);

private:
    MeowFn m_fn;
};

extern "C"
{

void* chat_new(char* (*meow)(const char*));
void  chat_free(void* chat);
char* chat_process(void* chat, const char* input);

}
