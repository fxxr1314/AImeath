// filemgr.cpp — 文件管理器 C ABI 后端
// 提供目录列表功能（list），文件内容由前端直接 HTTP fetch

#include "app_api.hpp"
#include "filemgr.hpp"

#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <vector>
#include <algorithm>

#include <boost/json.hpp>

namespace fs = std::filesystem;
namespace json = boost::json;

static const fs::path kRoot = "desktop/public/home";

// 安全解析路径：防止目录穿越
static std::string resolvePath(const std::string& path)
{
    std::string rel;
    if (path == "/home" || path == "/")
        rel = "";
    else if (path.rfind("/home/", 0) == 0)
        rel = path.substr(5);
    else
        rel = path;

    if (!rel.empty() && rel[0] == '/')
        rel = rel.substr(1);

    std::error_code ec;
    fs::path full = fs::weakly_canonical(kRoot / rel, ec);
    if (ec) return {};

    fs::path root = fs::weakly_canonical(kRoot, ec);
    if (ec) return {};

    std::string s_full = full.string();
    std::string s_root = root.string();

    if (s_full.size() < s_root.size() ||
        s_full.compare(0, s_root.size(), s_root) != 0 ||
        (s_full.size() > s_root.size() && s_full[s_root.size()] != '/'))
        return {};

    return s_full;
}

// 获取文件浏览器 URL（从根相对路径计算）
static std::string fileUrl(const fs::path& full)
{
    std::error_code ec;
    fs::path root = fs::weakly_canonical(kRoot, ec);
    if (ec) return {};
    auto r = fs::relative(full, root, ec);
    if (ec) return {};
    return "/home/" + r.generic_string();
}

// 获取文件修改时间
static std::string modifiedTime(const fs::path& p)
{
    struct stat st;
    if (::stat(p.c_str(), &st)) return {};
    char buf[64];
    if (std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", std::localtime(&st.st_mtime)))
        return buf;
    return {};
}

// 列出目录
static json::array listDir(const std::string& path)
{
    std::string full = resolvePath(path);
    if (full.empty())
    {
        json::array a;
        a.push_back(json::object{{"type","error"},{"msg","invalid path"}});
        return a;
    }

    json::array entries;
    std::error_code ec;
    for (auto& e : fs::directory_iterator(full, ec))
    {
        json::object obj;
        obj["name"] = e.path().filename().string();

        if (e.is_directory())
        {
            obj["kind"] = "dir";
            obj["size"] = 0;
        }
        else
        {
            obj["kind"] = "file";
            auto sz = e.file_size(ec);
            obj["size"] = ec ? 0 : static_cast<std::int64_t>(sz);
        }

        obj["modified"] = modifiedTime(e.path());
        obj["url"] = fileUrl(e.path());
        entries.push_back(std::move(obj));
    }

    // 排序：目录在前，按名称排序
    std::sort(entries.begin(), entries.end(),
        [](const json::value& a, const json::value& b)
        {
            auto& oa = a.as_object();
            auto& ob = b.as_object();
            bool ad = oa.at("kind").as_string() == "dir";
            bool bd = ob.at("kind").as_string() == "dir";
            if (ad != bd) return ad;
            return oa.at("name").as_string() < ob.at("name").as_string();
        });

    json::array result;
    result.push_back(json::object{
        {"type", "listing"},
        {"path", path},
        {"entries", std::move(entries)}
    });
    return result;
}

extern "C"
{

void* app_create(const char*)                  { return new int(0); }
void  app_destroy(void* p)                     { delete static_cast<int*>(p); }
void  app_free_string(char* s)                 { std::free(s); }
int   app_is_done(void*)                       { return 0; }

char* app_process(void*, const char* input_json)
{
    auto makeReply = [](json::array arr) -> char*
    {
        auto s = json::serialize(arr);
        auto* buf = static_cast<char*>(std::malloc(s.size() + 1));
        if (buf) std::memcpy(buf, s.data(), s.size() + 1);
        return buf;
    };

    try
    {
        auto val = json::parse(input_json);
        if (!val.is_object())
            return makeReply(json::array{json::object{{"type","error"},{"msg","not an object"}}});

        auto& obj = val.as_object();

        auto it = obj.find("action");
        if (it == obj.end() || !it->value().is_string())
            return makeReply(json::array{json::object{{"type","error"},{"msg","missing action"}}});

        std::string action(it->value().as_string());
        it = obj.find("path");
        std::string path = (it != obj.end() && it->value().is_string())
                           ? std::string(it->value().as_string()) : "/";

        if (action == "list")
            return makeReply(listDir(path));

        return makeReply(json::array{json::object{{"type","error"},{"msg","unknown action: "+action}}});
    }
    catch (const std::exception& e)
    {
        return makeReply(json::array{json::object{{"type","error"},{"msg",std::string(e.what())}}});
    }
}

} // extern "C"
