#ifndef VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#define VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#include "MaterialTextures.hpp"
#include "PropertiesManager.hpp"

namespace RtEngine {
    class MaterialInstance {
    public:
        MaterialInstance() = default;
        explicit MaterialInstance(const std::string& name) : name(name) {}
        virtual ~MaterialInstance() = default;

        virtual void* getResources(size_t* size, const std::shared_ptr<MaterialTextures<>>& tex_repo) = 0;
        virtual void loadResources(YAML::Node yaml_node) = 0;
        virtual YAML::Node writeResourcesToYaml() = 0;

        void setMaterialIndex(const uint32_t new_index) {
            material_index = new_index;
        }

        [[nodiscard]] uint32_t getMaterialIndex() const {
            return material_index;
        }

        virtual float getEmissionPower() { return 0.0f; }
        std::shared_ptr<PropertiesSection> getProperties() {
            if (properties == nullptr) {
                initializeInstanceProperties();
            }
            return properties;
        }

        std::string name = "";
    protected:
        virtual void initializeInstanceProperties() = 0;

        std::shared_ptr<PropertiesSection> properties;
        uint32_t material_index = 0;
    };

}

#endif //VULKAN_RAYTRACING_MATERIALINSTANCE_HPP