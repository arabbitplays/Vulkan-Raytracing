#ifndef VULKAN_RAYTRACING_MLMCPRESENTMODE_HPP
#define VULKAN_RAYTRACING_MLMCPRESENTMODE_HPP
#include <stdexcept>
#include <string>
#include <vector>

enum MlmcPresentMode {
    UNBIASED,
    BIASED,
    DIFF,
    COMBINED
};

class MlmcPresentModeConverter {
    public:
    static std::vector<std::string> getSelectionStrings() {
        return {
            "unbiased",
            "biased",
            "diff",
            "combined"
        };
    }

    static MlmcPresentMode fromString(const std::string& mode_string) {
        if (mode_string == "unbiased") {
            return UNBIASED;
        } else if (mode_string == "biased") {
            return BIASED;
        } else if (mode_string == "diff") {
            return DIFF;
        } else if (mode_string == "combined") {
            return COMBINED;
        }
        throw std::invalid_argument("Invalid MLMC mode string");
    }
};

#endif //VULKAN_RAYTRACING_MLMCPRESENTMODE_HPP