#ifndef VULKAN_RAYTRACING_MLMCMODE_HPP
#define VULKAN_RAYTRACING_MLMCMODE_HPP
#include <stdexcept>
#include <string>
#include <vector>

enum MlmcMode {
    PATH_LENGTH,
};

class MlmcModeConverter {
public:
    static std::vector<std::string> getSelectionStrings() {
        return {
            "path length",
        };
    }

    static MlmcMode fromString(const std::string &mode_string) {
        if (mode_string == "path length") {
            return PATH_LENGTH;
        }
        throw std::invalid_argument("Invalid MLMC mode string");
    }

    static uint32_t toIndex(MlmcMode mode) {
        return static_cast<uint32_t>(mode);
    }
};
#endif //VULKAN_RAYTRACING_MLMCMODE_HPP
