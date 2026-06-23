#include <gtest/gtest.h>
#include "game_base.hpp"
#include "game_api.hpp"

class TestGame : public Game
{
    int m_score = 0;
    bool m_over = false;
    std::string m_state;
public:
    void tick(int a) override
    {
        if (m_over) return;
        m_score += a;
        m_state = "tick:" + std::to_string(a);
    }
    bool isOver() const override { return m_over; }
    int score() const override { return m_score; }
    std::string getState() const override { return m_state; }
    void setOver(bool o) { m_over = o; }
};

TEST(GameBaseTest, VirtualDispatch)
{
    TestGame g;
    Game* pg = &g;
    pg->tick(10);
    EXPECT_EQ(pg->score(), 10);
    EXPECT_EQ(pg->isOver(), false);
    EXPECT_EQ(pg->getState(), "tick:10");
}

TEST(GameBaseTest, TickIgnoredWhenOver)
{
    TestGame g;
    g.setOver(true);
    g.tick(5);
    EXPECT_EQ(g.score(), 0);
}

extern "C" {
    void* game_new(int w, int h) { return new TestGame; }
    GAME_API_COMMON()
}

TEST(GameApiTest, ApiDispatch)
{
    void* g = game_new(0, 0);
    ASSERT_NE(g, nullptr);
    game_tick(g, 7);
    EXPECT_EQ(game_get_score(g), 7);
    EXPECT_EQ(game_is_over(g), 0);
    char* s = game_get_state(g);
    EXPECT_STREQ(s, "tick:7");
    std::free(s);
    game_free(g);
}


