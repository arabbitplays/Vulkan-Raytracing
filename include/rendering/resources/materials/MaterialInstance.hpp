//
// Created by oschdi on 09.09.25.
//

#ifndef VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#define VULKAN_RAYTRACING_MATERIALINSTANCE_HPP

namespace RtEngine {
    class MaterialInstance {
    public:
        MaterialInstance() = default;

        uint32_t getMaterialDataIndex() const {
            return static_cast<uint32_t>(material_idx);
        }

        void setMaterialDataIndex(const uint32_t material_data_idx) {
            material_idx = material_data_idx;
        }

        bool hasValidMaterialDataIndex() const {
            return material_idx >= 0;
        }

    private:
        int32_t material_idx = -1;
    };
}

#endif //VULKAN_RAYTRACING_MATERIALINSTANCE_HPP