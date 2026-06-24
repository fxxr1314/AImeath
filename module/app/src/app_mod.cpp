#include "app_mod.hpp"
#include <dlfcn.h>
#include <iostream>
#include <boost/dll.hpp>

/// Try to load a shared library by name.
/// Falls back to absolute paths relative to the executable.
static boost::dll::shared_library try_load(const std::string& name)
{
    std::string soname = "lib" + name + ".so";

    // Retry with ec overload to avoid throwing exceptions
    auto try_one = [&](const std::string& path) -> boost::dll::shared_library {
        std::error_code ec;
        boost::dll::shared_library lib(path, ec);
        if (!ec) return lib;
        return {};
    };

    // Try bare name (uses LD_LIBRARY_PATH, RUNPATH, /etc/ld.so.cache, /usr/lib)
    auto lib = try_one(soname);
    if (lib) return lib;

    // Build absolute paths relative to executable
    boost::system::error_code ec;
    boost::filesystem::path exe = boost::filesystem::read_symlink("/proc/self/exe", ec);
    if (ec) return {};

    boost::filesystem::path exe_dir = exe.parent_path();

    struct { boost::filesystem::path base; const char* suffix; } fallbacks[] = {
        { exe_dir,                       "lib" },
        { exe_dir.parent_path(),         "output/lib" },
        { exe_dir,                       "output/lib" },
        { exe_dir.parent_path(),         "lib" },
    };

    for (auto& fb : fallbacks) {
        boost::filesystem::path full = fb.base / fb.suffix / soname;
        lib = try_one(full.string());
        if (lib) return lib;
    }

    return {};
}

AppModule AppModuleCache::load(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = m_cache.find(name);
    if (it != m_cache.end())
        return it->second;

    auto lib = try_load(name);
    if (!lib)
    {
        std::cerr << "failed to load lib" << name << ".so" << std::endl;
        return {};
    }

    AppModule m;
    m.lib = std::move(lib);

    try
    {
        m.app_create      = m.lib.get<void*(const char*)>("app_create");
        m.app_destroy     = m.lib.get<void(void*)>("app_destroy");
        m.app_process     = m.lib.get<char*(void*,const char*)>("app_process");
        m.app_free_string = m.lib.get<void(char*)>("app_free_string");
        m.app_is_done     = m.lib.get<int(void*)>("app_is_done");
    }
    catch (const boost::system::system_error& e)
    {
        std::cerr << "symbols not found in lib" << name << ".so: " << e.what() << std::endl;
        return {};
    }

    if (!m.app_create || !m.app_destroy || !m.app_process || !m.app_free_string || !m.app_is_done)
    {
        std::cerr << "required symbols not found in lib" << name << ".so" << std::endl;
        return {};
    }

    // Optional async symbols — NULL if app doesn't support them yet
    try {
        m.app_on_input        = m.lib.get<void(void*,const char*)>("app_on_input");
        m.app_set_output      = m.lib.get<void(void*,app_output_fn,void*)>("app_set_output");
        m.app_set_io_context   = m.lib.get<void(void*,void*)>("app_set_io_context");
    } catch (...) {
        // async API not available, will fall back to app_process
    }

    m_cache[name] = m;
    return m;
}

void AppModuleCache::evict(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_cache.erase(name);
}

void AppModuleCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    m_cache.clear();
}
