#pragma once
#include <string>

namespace Hotones::Scripting {

/// Represents a loadable .cup game package.
///
/// A .cup file is simply a ZIP renamed to .cup. CupPackage accepts either:
///   – a .cup / .zip archive  → extracted to a temp directory at open()
///   – a plain directory      → used as-is (handy during development)
///
/// The temp directory, if created, is automatically removed on close() / destruction.
class CupPackage {
public:
    CupPackage();
    ~CupPackage();

    // Non-copyable (owns a temp directory)
    CupPackage(const CupPackage&)            = delete;
    CupPackage& operator=(const CupPackage&) = delete;

    /// Open a .cup/.zip file or a plain directory.
    /// Returns true on success.
    bool open(const std::string& path);

    /// Close the package and clean up any temp directory.
    /// Called automatically by the destructor.
    void close();

    bool               isOpen()    const { return m_open; }
    const std::string& rootPath()  const { return m_rootPath; }

    /// Full path to init.lua inside the package root.
    std::string initScript() const;

private:
    bool extractZip(const std::string& zipPath, const std::string& outDir);

    std::string m_rootPath;        ///< root directory of the package
    std::string m_tempDir;         ///< non-empty when we created a temp dir
    bool        m_open = false;
};

} // namespace Hotones::Scripting
