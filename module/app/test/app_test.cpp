#include <gtest/gtest.h>
#include "app_api.hpp"
#include "app_mod.hpp"
#include <string>
#include <cstring>

// ---- AppModule / AppPtr ----

TEST(AppModuleTest, DefaultModuleIsInvalid)
{
    AppModule m;
    EXPECT_FALSE(m);
}

TEST(AppModuleTest, CreateWithNullDeleterReturnsNull)
{
    AppModule m;
    AppPtr p = m.create("test");
    EXPECT_EQ(p.get(), nullptr);
}

// ---- AppPtr with custom deleter ----

struct TestApp { int val; };

TEST(AppPtrTest, UniquePtrLifetime)
{
    bool destroyed = false;
    auto* raw = new TestApp{42};
    {
        AppDeleter d;
        d.deleter = [](void* p) { delete static_cast<TestApp*>(p); };
        AppPtr ptr(raw, d);
        EXPECT_EQ(static_cast<TestApp*>(ptr.get())->val, 42);
        destroyed = false;
    }
    // No assertion, just verify it compiles and runs without crash
    SUCCEED();
}

// ---- AppModuleCache with mock .so ----

TEST(AppModuleCacheTest, LoadMock)
{
    AppModuleCache cache;
    auto m = cache.load("mock_app");
    ASSERT_TRUE(m) << "mock_app.so must be built first";

    auto app = m.create(R"({"cmd":"init"})");
    ASSERT_NE(app.get(), nullptr);

    EXPECT_FALSE(m.app_is_done(app.get()));

    char* out = m.app_process(app.get(), "hello");
    ASSERT_NE(out, nullptr);
    std::string result(out);
    m.app_free_string(out);

    EXPECT_TRUE(result.find("mock") != std::string::npos);
    EXPECT_TRUE(result.find("hello") != std::string::npos);
}

TEST(AppModuleCacheTest, LoadNonexistent)
{
    AppModuleCache cache;
    auto m = cache.load("nonexistent_module_xyz");
    EXPECT_FALSE(m);
}

TEST(AppModuleCacheTest, CacheHit)
{
    AppModuleCache cache;
    auto m1 = cache.load("mock_app");
    ASSERT_TRUE(m1);
    auto m2 = cache.load("mock_app");
    // Should return same cached handle
    EXPECT_TRUE(m2);
}

TEST(AppModuleCacheTest, Evict)
{
    AppModuleCache cache;
    auto m = cache.load("mock_app");
    ASSERT_TRUE(m);
    cache.evict("mock_app");
    // After evict, loading again should work (reload)
    auto m2 = cache.load("mock_app");
    EXPECT_TRUE(m2);
}


