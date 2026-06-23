#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <boost/dll/shared_library.hpp>
#include <boost/noncopyable.hpp>

struct AppDeleter
{
    void (*deleter)(void*) = nullptr;
    void operator()(void* a) const { if (deleter) deleter(a); }
};

using AppPtr = std::unique_ptr<void, AppDeleter>;

struct AppModule
{
    boost::dll::shared_library lib;
    void* (*app_create)(const char*) = nullptr;
    void  (*app_destroy)(void*) = nullptr;
    char* (*app_process)(void*, const char*) = nullptr;
    void  (*app_free_string)(char*) = nullptr;
    int   (*app_is_done)(void*) = nullptr;

    explicit operator bool() const { return lib.is_loaded(); }

    AppPtr create(const std::string& config) const
    {
        if (!app_create || !app_destroy) return {nullptr, AppDeleter{nullptr}};
        return {app_create(config.c_str()), AppDeleter{app_destroy}};
    }
};

/// Thread-safe cache for loaded app modules.
class AppModuleCache : private boost::noncopyable
{
public:
    AppModule load(const std::string& name);
    void evict(const std::string& name);
    void clear();

private:
    std::unordered_map<std::string, AppModule> m_cache;
    std::mutex m_mtx;
};
