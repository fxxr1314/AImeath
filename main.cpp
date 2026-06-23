#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>

#include <boost/json.hpp>

#include "app_mod.hpp"

extern "C" void run_netconn_demo();

// ---- Game runners using unified C ABI ----

static AppModuleCache s_cache;

static void run_snake()
{
    auto m = s_cache.load("snake");
    if (!m) { std::cerr << "snake: load failed" << std::endl; return; }

    auto app = m.create(R"({"action":"new_game","width":20,"height":20})");
    if (!app) { std::cerr << "snake: create failed" << std::endl; return; }

    int dir = 3; // right
    for (int i = 0; i < 50; ++i)
    {
        if (m.app_is_done(app.get())) break;

        boost::json::object tick;
        tick["action"] = "tick";
        tick["value"]  = dir;
        std::string msg = boost::json::serialize(tick);

        char* out = m.app_process(app.get(), msg.c_str());
        if (out) m.app_free_string(out);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Get final state
    if (!m.app_is_done(app.get()))
    {
        boost::json::object end;
        end["action"] = "end_game";
        char* out = m.app_process(app.get(), boost::json::serialize(end).c_str());
        if (out) m.app_free_string(out);
    }

    std::cout << "=== Snake ===" << std::endl;
    std::cout << "Done" << std::endl;
}

static void run_gomoku()
{
    auto m = s_cache.load("gomoku");
    if (!m) { std::cerr << "gomoku: load failed" << std::endl; return; }

    auto app = m.create(R"({"action":"new_game","width":15,"height":15})");
    if (!app) { std::cerr << "gomoku: create failed" << std::endl; return; }

    int moves[] = { 3*15+7, 0*15+0, 4*15+7, 1*15+0, 5*15+7, 2*15+0, 6*15+7, 3*15+1, 7*15+7 };

    for (int pos : moves)
    {
        if (m.app_is_done(app.get())) break;

        boost::json::object tick;
        tick["action"] = "tick";
        tick["value"]  = pos;
        char* out = m.app_process(app.get(), boost::json::serialize(tick).c_str());
        if (out) m.app_free_string(out);
    }

    std::cout << "=== Gomoku ===" << std::endl;
    std::cout << "Done" << std::endl;
}

static void run_pacman()
{
    auto m = s_cache.load("pacman");
    if (!m) { std::cerr << "pacman: load failed" << std::endl; return; }

    auto app = m.create(R"({"action":"new_game","width":20,"height":20})");
    if (!app) { std::cerr << "pacman: create failed" << std::endl; return; }

    int steps_in_dir = 5;
    int dirs[] = { 3, 2, 1, 2 }; // right, down, left, down
    for (int phase = 0; phase < 8; ++phase)
    {
        if (m.app_is_done(app.get())) break;
        int d = dirs[phase % 4];
        for (int i = 0; i < steps_in_dir; ++i)
        {
            if (m.app_is_done(app.get())) break;
            boost::json::object tick;
            tick["action"] = "tick";
            tick["value"]  = d;
            char* out = m.app_process(app.get(), boost::json::serialize(tick).c_str());
            if (out) m.app_free_string(out);
        }
    }

    std::cout << "=== Pac-Man ===" << std::endl;
    std::cout << "Done" << std::endl;
}

static void run_go()
{
    auto m = s_cache.load("go");
    if (!m) { std::cerr << "go: load failed" << std::endl; return; }

    auto app = m.create(R"({"action":"new_game","width":19,"height":19})");
    if (!app) { std::cerr << "go: create failed" << std::endl; return; }

    int blacks[] = { 3*19+3, 3*19+15, 15*19+3, 15*19+15 };
    int whites[] = { 9*19+9, 9*19+8, 9*19+10, 8*19+9, 10*19+9 };

    for (int i = 0; i < 4; ++i)
    {
        if (m.app_is_done(app.get())) break;
        boost::json::object tb; tb["action"] = "tick"; tb["value"] = blacks[i];
        char* ob = m.app_process(app.get(), boost::json::serialize(tb).c_str());
        if (ob) m.app_free_string(ob);

        if (m.app_is_done(app.get())) break;
        boost::json::object tw; tw["action"] = "tick"; tw["value"] = whites[i];
        char* ow = m.app_process(app.get(), boost::json::serialize(tw).c_str());
        if (ow) m.app_free_string(ow);
    }

    if (!m.app_is_done(app.get()))
    {
        boost::json::object tw; tw["action"] = "tick"; tw["value"] = whites[4];
        char* ow = m.app_process(app.get(), boost::json::serialize(tw).c_str());
        if (ow) m.app_free_string(ow);

        // Pass
        for (int i = 0; i < 2; ++i)
        {
            boost::json::object tp; tp["action"] = "tick"; tp["value"] = -1;
            char* op = m.app_process(app.get(), boost::json::serialize(tp).c_str());
            if (op) m.app_free_string(op);
        }
    }

    std::cout << "=== Go ===" << std::endl;
    std::cout << "Done" << std::endl;
}

static void run_chat()
{
    auto m = s_cache.load("chat");
    if (!m) { std::cerr << "chat: load failed" << std::endl; return; }

    auto app = m.create("{\"text\":\"hello\"}");
    if (!app) { std::cerr << "chat: create failed" << std::endl; return; }

    char* r1 = m.app_process(app.get(), "{\"text\":\"Hello\"}");
    char* r2 = m.app_process(app.get(), "{\"text\":\"\\u4f60\\u597d\\u4e16\\u754c\"}");

    std::cout << "=== Chat ===" << std::endl;
    if (r1) std::cout << "Hello -> " << r1 << std::endl;
    if (r2) std::cout << "\\u4f60\\u597d\\u4e16\\u754c -> " << r2 << std::endl;
    if (r1) m.app_free_string(r1);
    if (r2) m.app_free_string(r2);
}

static void run_netconn()
{
    std::cout << "=== NetConn (Core) ===" << std::endl;
    run_netconn_demo();
}

int main()
{
    run_netconn();
    run_chat();
    run_pacman();
    run_gomoku();
    run_snake();
    run_go();
    return 0;
}
