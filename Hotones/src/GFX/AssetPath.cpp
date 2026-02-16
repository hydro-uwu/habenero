#include "AssetPath.hpp"
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <vector>
#else
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#endif

static std::string GetExecutableDir()
{
#ifdef _WIN32
    std::vector<char> buf(MAX_PATH);
    DWORD len = GetModuleFileNameA(NULL, buf.data(), (DWORD)buf.size());
    if (len == 0) return std::string();
    std::string path(buf.data(), len);
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return std::string();
    return path.substr(0, pos);
#else
    char buf[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len == -1) return std::string();
    buf[len] = '\0';
    std::string path(buf);
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return std::string();
    return path.substr(0, pos);
#endif
}

static bool IsAbsolutePath(const std::string &p)
{
#ifdef _WIN32
    // Absolute: starts with X:\ or \\
    if (p.size() >= 2 && p[1] == ':') return true;
    if (!p.empty() && (p[0] == '\\' || p[0] == '/')) return true;
    return false;
#else
    return !p.empty() && p[0] == '/';
#endif
}

std::string ResolveAssetPath(const std::string &assetPath)
{
    if (assetPath.empty()) return assetPath;
    if (IsAbsolutePath(assetPath)) return assetPath;

    std::string exeDir = GetExecutableDir();
    if (exeDir.empty()) return assetPath;

#ifdef _WIN32
    char sep = '\\';
#else
    char sep = '/';
#endif
    std::string out = exeDir;
    if (out.back() != sep) out.push_back(sep);
    out += assetPath;
    return out;
}
