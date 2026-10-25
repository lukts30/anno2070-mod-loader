#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace anno2070 {
inline std::string normalize_game_path(const std::filesystem::path &path) {
  auto result = path.lexically_normal().generic_u8string();
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}
} // namespace anno2070
