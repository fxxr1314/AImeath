#include <benchmark/benchmark.h>
#include "agent_server.hpp"

static void BM_AgentCreateDestroy(benchmark::State& state) {
    for (auto _ : state) {
        auto ptr = std::make_shared<agent::AgentServer>();
        ptr->destroy();
    }
}
BENCHMARK(BM_AgentCreateDestroy);

static void BM_AgentOpenApp(benchmark::State& state) {
    auto ptr = std::make_shared<agent::AgentServer>();
    std::string dummy;
    ptr->setOutput([](void* udata, const char*) {}, &dummy);
    for (auto _ : state) {
        ptr->openApp("snake", "{}");
    }
    ptr->destroy();
}
BENCHMARK(BM_AgentOpenApp);

static void BM_AgentControlApp(benchmark::State& state) {
    auto ptr = std::make_shared<agent::AgentServer>();
    std::string dummy;
    ptr->setOutput([](void* udata, const char*) {}, &dummy);
    for (auto _ : state) {
        ptr->controlApp("snake", "{\"value\":3}");
    }
    ptr->destroy();
}
BENCHMARK(BM_AgentControlApp);

static void BM_AgentCloseApp(benchmark::State& state) {
    auto ptr = std::make_shared<agent::AgentServer>();
    std::string dummy;
    ptr->setOutput([](void* udata, const char*) {}, &dummy);
    for (auto _ : state) {
        ptr->closeApp("snake");
    }
    ptr->destroy();
}
BENCHMARK(BM_AgentCloseApp);

static void BM_AgentOnInputStop(benchmark::State& state) {
    for (auto _ : state) {
        auto ptr = std::make_shared<agent::AgentServer>();
        std::string dummy;
        ptr->setOutput([](void* udata, const char*) {}, &dummy);
        ptr->onInput("{\"action\":\"stop\"}");
        ptr->destroy();
    }
}
BENCHMARK(BM_AgentOnInputStop);

BENCHMARK_MAIN();
