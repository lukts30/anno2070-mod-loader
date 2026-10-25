#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace anno2070 {
inline std::string normalize_game_path(const std::filesystem::path &path) {
  auto result = path.generic_u8string();
  while (result.rfind("./", 0) == 0)
    result.erase(0, 2);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}
} // namespace anno2070
