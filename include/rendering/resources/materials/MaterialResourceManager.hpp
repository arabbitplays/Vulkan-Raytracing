//
// Created by oschdi on 08.09.25.
//

#ifndef VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP
#define VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP

#include "ResourceBuilder.hpp"
#include "VulkanContext.hpp"


namespace RtEngine {
    struct MaterialResources {

    };

    template <typename T>
    concept ConcreteResources = std::is_base_of_v<MaterialResources, T>;

    template<ConcreteResources Resources>
    class MaterialResourceManager {
    public:
        MaterialResourceManager() = default;
        explicit MaterialResourceManager(const std::shared_ptr<VulkanContext> &vulkan_context) : vulkan_context(vulkan_context) { }

        std::shared_ptr<AllocatedBuffer> getMaterialBuffer() {
            // TODO Optimization recreate only when needed
            if (material_buffer != nullptr) {
                vulkan_context->resource_builder->destroyBuffer(*material_buffer);
            }

            createMaterialBuffer();
            return material_buffer;
        }

        // returns the index into the material buffer or -1 is not a duplicate
        int32_t isDuplicate(const std::shared_ptr<Resources> &resources) {
            for (uint32_t i = 0; i < material_resources.size(); i++) {
                if (*resources == *material_resources[i]) {
                    return static_cast<int32_t>(i);
                }
            }
            return -1;
        }

        // returns the index into the material buffer
        uint32_t addResources(const std::shared_ptr<Resources> &resources) {
            material_resources.push_back(resources);
            return material_resources.size() - 1;
        }

        std::vector<std::shared_ptr<Resources>> getResources() {
            return material_resources;
        }

        void destroyResources() const {
            if (material_buffer != nullptr)
                vulkan_context->resource_builder->destroyBuffer(*material_buffer);
        }

        void reset() {
            material_resources.clear();
        };

    private:
        void createMaterialBuffer() {
            std::vector<Resources> material_data{};
            for (const auto & resources : material_resources) {
                material_data.push_back(*resources);
            }
            material_buffer = std::make_shared<AllocatedBuffer>(
                vulkan_context->resource_builder->stageMemoryToNewBuffer(
                    material_data.data(), material_data.size() * sizeof(Resources),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            );
        }

        std::shared_ptr<VulkanContext> vulkan_context;

        std::shared_ptr<AllocatedBuffer> material_buffer{};
        std::vector<std::shared_ptr<Resources>> material_resources{};
    };
}



#endif //VULKAN_RAYTRACING_MATERIALRESOURCEMANAGER_HPP