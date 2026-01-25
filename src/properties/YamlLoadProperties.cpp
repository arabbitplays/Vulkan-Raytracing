#include "../../include/properties/YamlLoadProperties.hpp"

namespace RtEngine {
    YamlLoadProperties::YamlLoadProperties(std::string config_path) {
        nodes.push_back(YAML::LoadFile(config_path));
    }

    bool YamlLoadProperties::startChild(const std::string &name) {
        YAML::Node child_node;
        if (getConfigValue(name, &child_node)) {
            nodes.push_back(child_node);
            return true;
        }
        return false;
    }

    bool YamlLoadProperties::endChild() {
        assert(nodes.size() > 1);
        nodes.pop_back();
        return true;
    }

    bool YamlLoadProperties::addBool(const std::string &name, bool *var) {
        return getConfigValue<bool>(name, var);
    }

    bool YamlLoadProperties::addInt(const std::string &name, int32_t *var, int32_t min, int32_t max, uint32_t flags) {
        return getRangeConfigValue<int32_t>(name, var, min, max);
    }

    bool YamlLoadProperties::addInt(const std::string &name, int32_t *var, uint32_t flags) {
        return getConfigValue<int32_t>(name, var);
    }

    bool YamlLoadProperties::addUint(const std::string &name, uint32_t *var, uint32_t min, uint32_t max, uint32_t flags) {
        return getRangeConfigValue<uint32_t>(name, var, min, max);
    }

    bool YamlLoadProperties::addUint(const std::string &name, uint32_t *var, uint32_t flags) {
        return getConfigValue<uint32_t>(name, var);
    }

    bool YamlLoadProperties::addFloat(const std::string &name, float *var, float min, float max, uint32_t flags) {
        return getConfigValue<float>(name, var);
    }

    bool YamlLoadProperties::addFloat(const std::string &name, float *var, uint32_t flags) {
        return getConfigValue<float>(name, var);
    }

    bool YamlLoadProperties::addString(const std::string &name, std::string *var, uint32_t flags) {
        return getConfigValue<std::string>(name, var);
    }

    bool YamlLoadProperties::addVector(const std::string &name, glm::vec3 *var, uint32_t flags) {
        return getConfigValue<glm::vec3>(name, var);
    }

    bool YamlLoadProperties::addVector(const std::string &name, glm::vec4 *var, uint32_t flags) {
        return getConfigValue<glm::vec4>(name, var);
    }

    bool YamlLoadProperties::addSelection(const std::string &name, std::string *var, std::vector<std::string> selection_options,
        uint32_t flags) {

        std::string select;
        bool found = getConfigValue<std::string>(name, &select);

        if (!found) {
            return false;
        }

        for (const auto & selection_option : selection_options) {
            if (select == selection_option) {
                *var = select;
                return true;
            }
        }

        SPDLOG_WARN("{} is not a valid value for property {}", select, name);
        return false;
    }
} // RtEngine