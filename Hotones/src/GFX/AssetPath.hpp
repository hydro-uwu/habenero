#pragma once

#include <string>

// Resolve an asset path relative to the executable directory when a relative path is provided.
std::string ResolveAssetPath(const std::string &assetPath);
