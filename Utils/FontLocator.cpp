#include "Utils/FontLocator.hpp"

#include <filesystem>
#include <vector>

namespace Game {

std::string locateFontFile() {
  const std::vector<std::string> candidates = {
      "assets/fonts/DejaVuSansMono.ttf",
      "C:/Windows/Fonts/consola.ttf",
      "C:/Windows/Fonts/segoeui.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
      "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
      "/System/Library/Fonts/Menlo.ttc"};

  for (const auto& path : candidates) {
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  return {};
}

}  // namespace Game
