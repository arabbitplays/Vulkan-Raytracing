#include "../../include/properties/YamlDumpProperties.hpp"
#include "YAML_glm.hpp"

namespace RtEngine {
    YamlDumpProperties::YamlDumpProperties(const YAML::Node &root_node) {
        nodes.push_back(root_node);
    }

    bool YamlDumpProperties::startChild(const std::string &name) {
        YAML::Node map_node(YAML::NodeType::Map);
        nodes.back()[name] = map_node;
        nodes.push_back(map_node);
        return true;
    }

    void YamlDumpProperties::endChild() {
        nodes.pop_back();
    }

    bool YamlDumpProperties::addBool(const std::string &name, bool *var) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addInt(const std::string &name, int32_t *var, int32_t min, int32_t max, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addInt(const std::string &name, int32_t *var, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addUint(const std::string &name, uint32_t *var, uint32_t min, uint32_t max,
        uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addUint(const std::string &name, uint32_t *var, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addFloat(const std::string &name, float *var, float min, float max, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addFloat(const std::string &name, float *var, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addString(const std::string &name, std::string *var, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }

    bool YamlDumpProperties::addVector(const std::string &name, glm::vec3 *var, uint32_t flags) {
        nodes.back()[name] = YAML::convert<glm::vec3>::encode(*var);
        return true;
    }

    bool YamlDumpProperties::addVector(const std::string &name, glm::vec4 *var, uint32_t flags) {
        nodes.back()[name] = YAML::convert<glm::vec4>::encode(*var);
        return true;
    }

    bool YamlDumpProperties::addSelection(const std::string &name, std::string *var,
        std::vector<std::string> selection_options, uint32_t flags) {
        nodes.back()[name] = *var;
        return true;
    }
}
