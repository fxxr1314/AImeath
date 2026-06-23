#include <benchmark/benchmark.h>
#include "direction.hpp"

static void BM_DirectionIsOpposite(benchmark::State& state)
{
    for (auto _ : state)
        benchmark::DoNotOptimize(isOppositeDir(Direction::UP, Direction::DOWN));
}
BENCHMARK(BM_DirectionIsOpposite);

static void BM_DirectionApplyDir(benchmark::State& state)
{
    int x = 0, y = 0;
    for (auto _ : state)
    {
        applyDir(Direction::RIGHT, x, y);
        benchmark::DoNotOptimize(x);
        benchmark::DoNotOptimize(y);
    }
}
BENCHMARK(BM_DirectionApplyDir);

BENCHMARK_MAIN();
