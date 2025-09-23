#ifndef VULKAN_RAYTRACING_RAYTRACINGPIPELINE_HPP
#define VULKAN_RAYTRACING_RAYTRACINGPIPELINE_HPP
#include "Pipeline.hpp"

namespace RtEngine {
    class RaytracingPipeline : public Pipeline {
    public:
        explicit RaytracingPipeline(const std::shared_ptr<VulkanContext> &context) : Pipeline(context) { }

        void build() override;
        void createShaderBindingTables(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

        void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage,
                            VkRayTracingShaderGroupTypeKHR shaderGroup);

        AllocatedBuffer raygenShaderBindingTable;
        AllocatedBuffer missShaderBindingTable;
        AllocatedBuffer hitShaderBindingTable;

    private:
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};

    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RAYTRACINGPIPELINE_HPP