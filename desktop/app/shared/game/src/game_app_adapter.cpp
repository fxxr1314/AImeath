/// game_app_adapter.cpp
/// Unified app C ABI implementation for games.
/// Each game .so compiles this file so it exports app_create, app_process, etc.
/// The game must provide game_new, game_free, game_tick, game_get_state etc.
/// (typically via GAME_API_COMMON()).

#include "app_api.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>

#include <boost/json.hpp>

// ---- inline JSON helpers (no dependency on module/core) ----

static std::string jsonGetStr(const std::string& msg, const std::string& key)
{
    try {
        auto val = boost::json::parse(msg);
        if (!val.is_object()) return {};
        auto& obj = val.as_object();
        auto it = obj.find(key);
        if (it == obj.end()) return {};
        auto& v = it->value();
        return v.is_string() ? std::string(v.as_string()) :
               v.is_int64() || v.is_uint64() || v.is_double() || v.is_bool()
                   ? boost::json::serialize(v) : std::string();
    } catch (...) { return {}; }
}

static int jsonGetInt(const std::string& msg, const std::string& key)
{
    try {
        auto val = boost::json::parse(msg);
        if (!val.is_object()) return 0;
        auto& obj = val.as_object();
        auto it = obj.find(key);
        if (it == obj.end()) return 0;
        auto r = boost::json::try_value_to<std::int64_t>(it->value());
        return r ? static_cast<int>(*r) : 0;
    } catch (...) { return 0; }
}

static std::string jsonError(const std::string& msg)
{
    boost::json::object obj;
    obj["type"] = "error";
    obj["msg"]  = msg;
    return boost::json::serialize(obj);
}

static std::string jsonOk()
{
    return boost::json::serialize(boost::json::object{{"type", "ok"}});
}

// Games provide these via their existing C ABI
extern "C"
{
    void* game_new(int w, int h);
    void  game_free(void* g);
    void  game_tick(void* g, int a);
    int   game_is_over(void* g);
    int   game_get_score(void* g);
    char* game_get_state(void* g);
}

struct GameApp
{
    void* game = nullptr;
    int width = 0, height = 0;
    bool paused = false;
    bool done = false;
};

static std::string buildArray(const boost::json::value& item)
{
    boost::json::array arr;
    arr.push_back(item);
    return boost::json::serialize(arr);
}

extern "C"
{

void* app_create(const char* /*config_json*/)
{
    return new GameApp();
}

void app_destroy(void* p)
{
    auto* app = static_cast<GameApp*>(p);
    if (app->game) game_free(app->game);
    delete app;
}

char* app_process(void* p, const char* input_json)
{
    auto* app = static_cast<GameApp*>(p);
    std::string msg(input_json);

    std::string action = jsonGetStr(msg, "action");
    if (action.empty()) action = jsonGetStr(msg, "type");

    if (action == "start" || action == "new_game")
    {
        if (app->game)
        {
            std::string err = buildArray(boost::json::parse(jsonError("game already running")));
            char* buf = static_cast<char*>(std::malloc(err.size() + 1));
            if (buf) std::memcpy(buf, err.data(), err.size() + 1);
            return buf;
        }

        int w = jsonGetInt(msg, "width");
        if (w <= 0) w = 20;
        int h = jsonGetInt(msg, "height");
        if (h <= 0) h = 20;
        app->width = w; app->height = h;

        app->game = game_new(w, h);
        app->paused = false;
        app->done = false;

        if (!app->game)
        {
            std::string err = buildArray(boost::json::parse(jsonError("game_new failed")));
            char* buf = static_cast<char*>(std::malloc(err.size() + 1));
            if (buf) std::memcpy(buf, err.data(), err.size() + 1);
            return buf;
        }

        char* state = game_get_state(app->game);
        std::string result = state ? buildArray(boost::json::parse(state)) : "[]";
        std::free(state);
        char* buf = static_cast<char*>(std::malloc(result.size() + 1));
        if (buf) std::memcpy(buf, result.data(), result.size() + 1);
        return buf;
    }
    else if (action == "tick" || action == "input")
    {
        if (!app->game)
        {
            std::string err = buildArray(boost::json::parse(jsonError("no game")));
            char* buf = static_cast<char*>(std::malloc(err.size() + 1));
            if (buf) std::memcpy(buf, err.data(), err.size() + 1);
            return buf;
        }
        if (app->paused)
        {
            char* buf = static_cast<char*>(std::malloc(3));
            if (buf) { buf[0] = '['; buf[1] = ']'; buf[2] = '\0'; }
            return buf;
        }

        int val = jsonGetInt(msg, "value");
        game_tick(app->game, val);
        char* state = game_get_state(app->game);
        std::string result = state ? buildArray(boost::json::parse(state)) : "[]";
        std::free(state);
        char* buf = static_cast<char*>(std::malloc(result.size() + 1));
        if (buf) std::memcpy(buf, result.data(), result.size() + 1);
        return buf;
    }
    else if (action == "pause")
    {
        app->paused = true;
        std::string ok = buildArray(boost::json::parse(jsonOk()));
        char* buf = static_cast<char*>(std::malloc(ok.size() + 1));
        if (buf) std::memcpy(buf, ok.data(), ok.size() + 1);
        return buf;
    }
    else if (action == "resume")
    {
        app->paused = false;
        std::string ok = buildArray(boost::json::parse(jsonOk()));
        char* buf = static_cast<char*>(std::malloc(ok.size() + 1));
        if (buf) std::memcpy(buf, ok.data(), ok.size() + 1);
        return buf;
    }
    else if (action == "end_game")
    {
        app->done = true;
        if (app->game) { game_free(app->game); app->game = nullptr; }
        std::string ok = buildArray(boost::json::parse(jsonOk()));
        char* buf = static_cast<char*>(std::malloc(ok.size() + 1));
        if (buf) std::memcpy(buf, ok.data(), ok.size() + 1);
        return buf;
    }
    else
    {
        std::string err = buildArray(boost::json::parse(jsonError("unknown action: " + action)));
        char* buf = static_cast<char*>(std::malloc(err.size() + 1));
        if (buf) std::memcpy(buf, err.data(), err.size() + 1);
        return buf;
    }
}

void app_free_string(char* str)
{
    std::free(str);
}

int app_is_done(void* p)
{
    return static_cast<GameApp*>(p)->done ? 1 : 0;
}

} // extern "C"
