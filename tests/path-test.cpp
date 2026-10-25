#include "../src/path.h"

#include <iostream>

int main() {
  const auto package =
      anno2070::normalize_game_path("data/config/game/assets.xml");
  for (const auto *native :
       {L"./data/config/game/Assets.xml", L".\\DATA\\config\\game\\Assets.xml",
        L"././data/config/game/Assets.xml"}) {
    if (anno2070::normalize_game_path(native) != package) {
      std::cerr << "Native path did not match the package path\n";
      return 1;
    }
  }
  if (anno2070::normalize_game_path("addondata/config/game/assets.xml") ==
      package)
    return 2;
  if (anno2070::normalize_game_path("data/config/game/../game/assets.xml") ==
      package)
    return 3;
  std::cout << "Native startup and package paths match\n";
  return 0;
}
