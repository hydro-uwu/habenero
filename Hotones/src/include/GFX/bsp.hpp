#pragma once
#include <vector>
#include <filesystem>
#include <raylib.h>

namespace Hotones {

// Load one or more raylib `Model` objects from a Quake BSP file.
// Returns an empty vector on failure.

std::vector<Model>
LoadModelsFromBSPFile(const std::filesystem::path& path);

} // namespace Hotones
