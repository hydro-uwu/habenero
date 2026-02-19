// CupPackage.cpp — .cup (zip) game-pack extractor
//
// DEPENDENCY: miniz — single-file, public-domain ZIP library
//   1. Download:  https://github.com/richgel999/miniz/releases  (grab miniz.h)
//   2. Place at:  src/include/miniz.h
//
// miniz is used as a single-header library: defining MINIZ_IMPLEMENTATION in
// this translation unit provides the full implementation with no separate .c
// compilation step needed.

#define MINIZ_IMPLEMENTATION
#include <miniz.h>

#include <filesystem>
#include <iostream>
#include <raylib.h>
#include "../include/Scripting/CupPackage.hpp"

namespace fs = std::filesystem;

namespace Hotones::Scripting {

CupPackage::CupPackage()  = default;
CupPackage::~CupPackage() { close(); }

bool CupPackage::open(const std::string& path)
{
    close();

    fs::path p(path);
    if (!fs::exists(p)) {
        TraceLog(LOG_ERROR, "[CupPackage] Path does not exist: %s", path.c_str());
        return false;
    }

    // ── Plain directory (dev / pre-extracted) ────────────────────────────────
    if (fs::is_directory(p)) {
        m_rootPath = fs::absolute(p).string();
        m_open     = true;
        TraceLog(LOG_INFO, "[CupPackage] Opened directory pack: %s", m_rootPath.c_str());
        return true;
    }

    // ── .cup / .zip archive ───────────────────────────────────────────────────
    std::string ext = p.extension().string();
    if (ext != ".cup" && ext != ".zip") {
        TraceLog(LOG_ERROR, "[CupPackage] Unknown extension '%s' — expected .cup, .zip, or a directory.", ext.c_str());
        return false;
    }

    // Extract into <tmp>/hotones_cup_<stem>/
    fs::path tmp = fs::temp_directory_path()
                 / ("hotones_cup_" + p.stem().string());

    if (fs::exists(tmp))
        fs::remove_all(tmp);
    fs::create_directories(tmp);

    if (!extractZip(path, tmp.string())) {
        fs::remove_all(tmp);
        return false;
    }

    m_tempDir  = tmp.string();
    m_rootPath = m_tempDir;
    m_open     = true;
    TraceLog(LOG_INFO, "[CupPackage] Extracted '%s' -> %s", p.filename().string().c_str(), m_tempDir.c_str());
    return true;
}

std::string CupPackage::initScript() const
{
    if (!m_open) return {};
    return (fs::path(m_rootPath) / "init.lua").string();
}

void CupPackage::close()
{
    if (!m_tempDir.empty()) {
        std::error_code ec;
        fs::remove_all(m_tempDir, ec);
        m_tempDir.clear();
    }
    m_rootPath.clear();
    m_open = false;
}

bool CupPackage::extractZip(const std::string& zipPath, const std::string& outDir)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_file(&zip, zipPath.c_str(), 0)) {
        TraceLog(LOG_ERROR, "[CupPackage] Failed to open archive: %s", zipPath.c_str());
        return false;
    }

    const mz_uint count = mz_zip_reader_get_num_files(&zip);
    bool ok = true;

    for (mz_uint i = 0; i < count; ++i) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&zip, i, &stat))
            continue;

        fs::path dest = fs::path(outDir) / stat.m_filename;

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            fs::create_directories(dest);
            continue;
        }

        fs::create_directories(dest.parent_path());

        if (!mz_zip_reader_extract_to_file(&zip, i, dest.string().c_str(), 0)) {
            TraceLog(LOG_ERROR, "[CupPackage] Failed to extract: %s", stat.m_filename);
            ok = false;
        }
    }

    mz_zip_reader_end(&zip);
    return ok;
}

} // namespace Hotones::Scripting
