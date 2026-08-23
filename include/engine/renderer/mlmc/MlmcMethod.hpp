#ifndef VULKAN_RAYTRACING_MLMCMODE_HPP
#define VULKAN_RAYTRACING_MLMCMODE_HPP
#include <stdexcept>
#include <string>
#include <vector>

enum MlmcMethod {
    PATH_LENGTH_PREFIX,
    SIMILARITY,
    SIMILARITY_SAME_PATH,
    SIMILARITY_SKIP_DETERMINISTIC,
    HOMO,
    HOMO_SAME_PATH,
    FIRST_ORDER_SIMILARITY,
    FIRST_ORDER_SIMILARITY_SAME_PATH,
    FIRST_ORDER_SIMILARITY_SKIP_DETERMINISTIC,
    PATH_LENGTH_RESAMPLE,
};

class MlmcMethodConverter {
public:
    static std::vector<std::string> getSelectionStrings() {
        return {
            "path length prefix",
            "similarity",
            "similarity same path",
            "similarity skip deterministic",
            "homo",
            "homo same path",
            "first order similarity",
            "first order similarity same path",
            "first order similarity skip deterministic",
            "path length resample",
        };
    }

    static MlmcMethod fromString(const std::string &mode_string) {
        if (mode_string == "path length prefix") {
            return PATH_LENGTH_PREFIX;
        }
        if (mode_string == "similarity") {
            return SIMILARITY;
        }
        if (mode_string == "similarity same path") {
            return SIMILARITY_SAME_PATH;
        }
        if (mode_string == "similarity skip deterministic") {
            return SIMILARITY_SKIP_DETERMINISTIC;
        }
        if (mode_string == "homo") {
            return HOMO;
        }
        if (mode_string == "homo same path") {
            return HOMO_SAME_PATH;
        }
        if (mode_string == "first order similarity") {
            return FIRST_ORDER_SIMILARITY;
        }
        if (mode_string == "first order similarity same path") {
            return FIRST_ORDER_SIMILARITY_SAME_PATH;
        }
        if (mode_string == "first order similarity skip deterministic") {
            return FIRST_ORDER_SIMILARITY_SKIP_DETERMINISTIC;
        }
        if (mode_string == "path length resample") {
            return PATH_LENGTH_RESAMPLE;
        }
        throw std::invalid_argument("Invalid MLMC method string");
    }

    static uint32_t toIndex(MlmcMethod mode) {
        return static_cast<uint32_t>(mode);
    }
};
#endif //VULKAN_RAYTRACING_MLMCMODE_HPP
