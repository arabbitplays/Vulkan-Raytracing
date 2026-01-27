#ifndef VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP
#define VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP
#include "VulkanContext.hpp"

namespace RtEngine {
    class ComputePipeline {
    public:
        ComputePipeline() { clear(); }
        ComputePipeline(const std::shared_ptr<VulkanContext> &context) : context(context) { clear(); }

        void build();

        void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage,
                            VkRayTracingShaderGroupTypeKHR shaderGroup);
        void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);
        void addPushConstant(uint32_t size, VkShaderStageFlags shaderStage);

        void clear();
        void destroy();

        VkPipeline getHandle() const;
        VkPipelineLayout getLayoutHandle() const;

    private:
        std::shared_ptr<VulkanContext> context;
        DeletionQueue deletionQueue;

        VkPipeline handle = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo;
        std::vector<VkPushConstantRange> pushConstants{};
        std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTEPIPELINE_HPP