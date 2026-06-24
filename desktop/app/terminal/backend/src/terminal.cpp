#include "app_api.hpp"
#include "terminal.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>
#include <array>

#include <boost/json.hpp>

namespace json = boost::json;

extern "C"
{

void* app_create(const char* config_json)
{
    (void)config_json;
    auto* ses = new TermSession();
    return ses;
}

void app_destroy(void* p)
{
    delete static_cast<TermSession*>(p);
}

void app_free_string(char* s)
{
    std::free(s);
}

int app_is_done(void* p)
{
    auto* ses = static_cast<TermSession*>(p);
    return ses->isAlive() ? 0 : 1;
}

char* app_process(void* p, const char* input_json)
{
    auto* ses = static_cast<TermSession*>(p);

    auto makeReply = [](json::array arr) -> char*
    {
        auto s = json::serialize(arr);
        auto* buf = static_cast<char*>(std::malloc(s.size() + 1));
        if (buf) std::memcpy(buf, s.data(), s.size() + 1);
        return buf;
    };

    try
    {
        auto val = json::parse(input_json);
        if (!val.is_object())
            return makeReply(json::array{json::object{{"type","error"},{"msg","not an object"}}});

        auto& obj = val.as_object();

        auto it = obj.find("action");
        if (it == obj.end() || !it->value().is_string())
            return makeReply(json::array{json::object{{"type","error"},{"msg","missing action"}}});

        std::string action(it->value().as_string());

        if (action == "exec")
        {
            auto cmdIt = obj.find("cmd");
            if (cmdIt == obj.end() || !cmdIt->value().is_string())
                return makeReply(json::array{json::object{{"type","error"},{"msg","missing cmd"}}});

            std::string cmd(cmdIt->value().as_string());

            if (!ses->start(cmd))
                return makeReply(json::array{json::object{{"type","error"},{"msg","forkpty failed"}}});

            usleep(150000); // 150ms for shell prompt to appear
            std::string output = ses->readOutput();
            return makeReply(json::array{json::object{
                {"type", "output"},
                {"text", output}
            }});
        }
        else if (action == "stdin")
        {
            auto dataIt = obj.find("data");
            if (dataIt == obj.end() || !dataIt->value().is_string())
                return makeReply(json::array{json::object{{"type","error"},{"msg","missing data"}}});

            std::string data(dataIt->value().as_string());
            ses->writeInput(data);
            std::string output = ses->readOutput();
            return makeReply(json::array{json::object{
                {"type", "output"},
                {"text", output}
            }});
        }
        else if (action == "stdout")
        {
            std::string output = ses->readPending();
            return makeReply(json::array{json::object{
                {"type", "output"},
                {"text", output}
            }});
        }
        else if (action == "resize")
        {
            int rows = 24, cols = 80;
            auto rit = obj.find("rows");
            if (rit != obj.end() && rit->value().is_int64())
                rows = (int)rit->value().as_int64();
            auto cit = obj.find("cols");
            if (cit != obj.end() && cit->value().is_int64())
                cols = (int)cit->value().as_int64();
            ses->setSize(rows, cols);
            return makeReply(json::array{});
        }

        return makeReply(json::array{json::object{{"type","error"},{"msg","unknown action: "+action}}});
    }
    catch (const std::exception& e)
    {
        return makeReply(json::array{json::object{{"type","error"},{"msg",std::string(e.what())}}});
    }
}

} // extern "C"
