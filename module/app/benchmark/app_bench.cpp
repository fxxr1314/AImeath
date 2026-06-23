#include <benchmark/benchmark.h>
#include "app_mod.hpp"
#include <string>

// Benchmark loading and unloading a mock .so
static void BM_AppModuleLoad(benchmark::State& state)
{
    for (auto _ : state)
    {
        AppModuleCache cache;
        auto m = cache.load("mock_app");
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_AppModuleLoad);

// Benchmark creating an app instance
static void BM_AppCreate(benchmark::State& state)
{
    AppModuleCache cache;
    auto m = cache.load("mock_app");
    for (auto _ : state)
    {
        auto app = m.create(R"({"bench":true})");
        benchmark::DoNotOptimize(app);
    }
}
BENCHMARK(BM_AppCreate);

// Benchmark app_process round-trip
static void BM_AppProcess(benchmark::State& state)
{
    AppModuleCache cache;
    auto m = cache.load("mock_app");
    auto app = m.create(R"({"bench":true})");
    for (auto _ : state)
    {
        char* out = m.app_process(app.get(), "ping");
        benchmark::DoNotOptimize(out);
        m.app_free_string(out);
    }
}
BENCHMARK(BM_AppProcess);

// Benchmark cache hit (no dlopen)
static void BM_AppCacheHit(benchmark::State& state)
{
    AppModuleCache cache;
    cache.load("mock_app");
    for (auto _ : state)
    {
        auto m = cache.load("mock_app");
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_AppCacheHit);

BENCHMARK_MAIN();
