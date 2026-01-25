//
// Created by oschdi on 25.01.26.
//

#ifndef VULKAN_RAYTRACING_PROPERTIES_HPP
#define VULKAN_RAYTRACING_PROPERTIES_HPP
#include <memory>
#include <unordered_map>

#include "Node.hpp"
#include "IProperties.hpp"

namespace RtEngine {
    class YamlLoadProperties final : public IProperties {
    public:
        explicit YamlLoadProperties(std::string config_path);
        explicit YamlLoadProperties(YAML::Node config_node);

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
        template<typename T>
        bool getConfigValue(const std::string &key, T* var) {
            if (!nodes.back()[key].IsDefined())
                return false;

            *var = nodes.back()[key].as<T>();
            return true;
        }

        template<typename T>
        bool getRangeConfigValue(const std::string &key, T* var, T min, T max) {
            if (!nodes.back()[key].IsDefined())
                return false;

            T value = nodes.back()[key].as<T>();
            *var = std::clamp(value, min, max);
            return true;
        }

        std::vector<YAML::Node> nodes;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_PROPERTIES_HPP