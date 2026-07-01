#pragma once

#include <memory>
#include <string>

typedef void (*app_output_fn)(void* userdata, const char* json);

struct AppDeleter
{
    void (*deleter)(void*) = nullptr;
    void operator()(void* a) const { if (deleter) deleter(a); }
};

using AppPtr = std::unique_ptr<void, AppDeleter>;

struct AppModule
{
    bool loaded = false;

    void* (*app_create)(const char*)  = nullptr;
    void  (*app_destroy)(void*)       = nullptr;
    int   (*app_is_done)(void*)       = nullptr;

    void  (*app_on_input)(void*, const char*)               = nullptr;
    void  (*app_set_output)(void*, app_output_fn, void*)    = nullptr;
    void  (*app_set_io_context)(void*, void*)               = nullptr;

    char* (*app_process)(void*, const char*) = nullptr;
    void  (*app_free_string)(char*)           = nullptr;

    explicit operator bool() const { return loaded && app_create && app_destroy && app_process; }

    bool is_async() const { return app_on_input && app_set_output; }

    AppPtr create(const std::string& config) const
    {
        if (!app_create || !app_destroy) return {nullptr, AppDeleter{nullptr}};
        return {app_create(config.c_str()), AppDeleter{app_destroy}};
    }
};

class IModuleCache
{
public:
    virtual ~IModuleCache() = default;
    virtual AppModule load(const std::string& name) = 0;
    virtual void evict(const std::string& name) = 0;
    virtual void clear() = 0;
};
