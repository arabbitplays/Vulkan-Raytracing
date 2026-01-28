#ifndef VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#define VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#include "MaterialTextures.hpp"
#include "ISerializable.hpp"
#include <yaml-cpp/yaml.h>
#include "YAML_glm.hpp"

namespace RtEngine {
    class MaterialInstance : public ISerializable {
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

        void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override = 0;

        std::string name = "";
    protected:
        uint32_t material_index = 0;
    };

}

#endif //VULKAN_RAYTRACING_MATERIALINSTANCE_HPP