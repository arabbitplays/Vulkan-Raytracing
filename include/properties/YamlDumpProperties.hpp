#ifndef VULKAN_RAYTRACING_YAMLDUMPPROPERTIES_HPP
#define VULKAN_RAYTRACING_YAMLDUMPPROPERTIES_HPP
#include "IProperties.hpp"
#include <yaml-cpp/yaml.h>

namespace RtEngine {
    class YamlDumpProperties final : public IProperties {
    public:
        explicit YamlDumpProperties(const YAML::Node &root_node);

        bool startChild(const std::string &name) override;

        void endChild() override;

        bool addBool(const std::string &name, bool *var) override;

        bool addInt(const std::string &name, int32_t *var, int32_t min, int32_t max, uint32_t flags) override;

        bool addInt(const std::string &name, int32_t *var, uint32_t flags) override;

        bool addUint(const std::string &name, uint32_t *var, uint32_t min, uint32_t max, uint32_t flags) override;

        bool addUint(const std::string &name, uint32_t *var, uint32_t flags) override;

        bool addFloat(const std::string &name, float *var, float min, float max, uint32_t flags) override;

        bool addFloat(const std::string &name, float *var, uint32_t flags) override;

        bool addString(const std::string &name, std::string *var, uint32_t flags) override;

        bool addVector(const std::string &name, glm::vec3 *var, uint32_t flags) override;

        bool addVector(const std::string &name, glm::vec4 *var, uint32_t flags) override;

        bool addSelection(const std::string &name, std::string *var, std::vector<std::string> selection_options,
            uint32_t flags) override;

    private:
        std::vector<YAML::Node> nodes{};
    };
}

#endif //VULKAN_RAYTRACING_YAMLDUMPPROPERTIES_HPP