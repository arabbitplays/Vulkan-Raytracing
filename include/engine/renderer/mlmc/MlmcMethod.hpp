#ifndef VULKAN_RAYTRACING_MLMCMODE_HPP
#define VULKAN_RAYTRACING_MLMCMODE_HPP
#include <stdexcept>
#include <string>
#include <vector>

enum MlmcMethod {
    PATH_LENGTH,
    SIMILARITY,
    SIMILARITY_SAME_PATH,
    SIMILARITY_SKIP_DETERMINISTIC,
    SIMILARITY_SKIP_RANDOM,
    HOMO,
};

class MlmcMethodConverter {
public:
    static std::vector<std::string> getSelectionStrings() {
        return {
            "path length",
            "similarity",
            "same path similarity",
            "skip deterministic similarity",
            "skip random similarity",
            "homo",
        };
    }

    static MlmcMethod fromString(const std::string &mode_string) {
        if (mode_string == "path length") {
            return PATH_LENGTH;
        }
        if (mode_string == "similarity") {
            return SIMILARITY;
        }
        if (mode_string == "same path similarity") {
            return SIMILARITY_SAME_PATH;
        }
        if (mode_string == "skip deterministic similarity") {
            return SIMILARITY_SKIP_DETERMINISTIC;
        }
        if (mode_string == "skip random similarity") {
            return SIMILARITY_SKIP_RANDOM;
        }
        if (mode_string == "homo") {
            return HOMO;
        }
        throw std::invalid_argument("Invalid MLMC method string");
    }

    static uint32_t toIndex(MlmcMethod mode) {
        return static_cast<uint32_t>(mode);
    }
};
#endif //VULKAN_RAYTRACING_MLMCMODE_HPP
