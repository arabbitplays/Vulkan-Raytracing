#ifndef VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#define VULKAN_RAYTRACING_MATERIALINSTANCE_HPP

#include "ISerializable.hpp"

namespace RtEngine {
    class Material;

    class MaterialInstance : ISerializable {
    public:
        MaterialInstance() = default;
        virtual ~MaterialInstance() = default;
        virtual void attachTo(Material& m) = 0;  // Double dispatch entry point

        [[nodiscard]] uint32_t getMaterialDataIndex() const {
            return static_cast<uint32_t>(material_idx);
        }

        void setMaterialDataIndex(const uint32_t material_data_idx) {
            material_idx = static_cast<int32_t>(material_data_idx);
        }

        [[nodiscard]] bool hasValidMaterialDataIndex() const {
            return material_idx >= 0;
        }

        std::shared_ptr<PropertiesSection> getProperties() override {
            if (!properties) {
               initializeProperties();
            }
            return properties;
        }

    protected:
        virtual void initializeProperties() = 0;
        std::shared_ptr<PropertiesSection> properties;

        int32_t material_idx = -1;

    };
}

#endif //VULKAN_RAYTRACING_MATERIALINSTANCE_HPP