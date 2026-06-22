#include <benchmark/benchmark.h>
#include "chat.hpp"

static std::string meow(const std::string& text)
{
    return text + "\xe5\x96\xb5";
}

static void BM_ChatProcessShort(benchmark::State& state)
{
    Chat chat(meow);
    for (auto _ : state)
    {
        auto result = chat.process("hello");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ChatProcessShort);

static void BM_ChatProcessLong(benchmark::State& state)
{
    std::string input(1000, 'x');
    Chat chat(meow);
    for (auto _ : state)
    {
        auto result = chat.process(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ChatProcessLong);

static void BM_ChatProcessChinese(benchmark::State& state)
{
    Chat chat(meow);
    for (auto _ : state)
    {
        auto result = chat.process("今天天气不错，适合写代码");
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ChatProcessChinese);

BENCHMARK_MAIN();
