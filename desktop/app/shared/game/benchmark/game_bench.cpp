#include <benchmark/benchmark.h>
#include "game_base.hpp"

class BenchGame : public Game
{
    int m_score = 0;
    bool m_over = false;
public:
    void tick(int a) override { m_score += a; }
    bool isOver() const override { return m_over; }
    int score() const override { return m_score; }
    std::string getState() const override { return std::to_string(m_score); }
};

static void BM_GameVirtualTick(benchmark::State& state)
{
    BenchGame g;
    for (auto _ : state)
        g.tick(1);
}
BENCHMARK(BM_GameVirtualTick);

BENCHMARK_MAIN();
