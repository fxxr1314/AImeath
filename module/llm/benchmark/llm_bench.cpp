#include <benchmark/benchmark.h>
#include "llm_client.hpp"
#include <boost/asio.hpp>

static void BM_LlmClientCreateDestroy(benchmark::State& state)
{
    boost::asio::io_context io;
    for (auto _ : state)
    {
        auto client = std::make_shared<LlmClient>(io);
        benchmark::DoNotOptimize(client);
    }
}
BENCHMARK(BM_LlmClientCreateDestroy);

static void BM_LlmClientCreateCancel(benchmark::State& state)
{
    boost::asio::io_context io;
    for (auto _ : state)
    {
        auto client = std::make_shared<LlmClient>(io);
        client->cancel();
        benchmark::DoNotOptimize(client);
    }
}
BENCHMARK(BM_LlmClientCreateCancel);

BENCHMARK_MAIN();
