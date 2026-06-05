#ifndef VULKAN_RAYTRACING_MLMCMODE_HPP
#define VULKAN_RAYTRACING_MLMCMODE_HPP
#include <stdexcept>
#include <string>
#include <vector>

enum MlmcMethod {
    PATH_LENGTH,
    SIMILARITY,
};

class MlmcMethodConverter {
public:
    static std::vector<std::string> getSelectionStrings() {
        return {
            "path length",
            "similarity",
        };
    }

    static MlmcMethod fromString(const std::string &mode_string) {
        if (mode_string == "path length") {
            return PATH_LENGTH;
        }
        if (mode_string == "similarity") {
            return SIMILARITY;
        }
        throw std::invalid_argument("Invalid MLMC method string");
    }

    static uint32_t toIndex(MlmcMethod mode) {
        return static_cast<uint32_t>(mode);
    }
};
#endif //VULKAN_RAYTRACING_MLMCMODE_HPP
