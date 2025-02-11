//
// Created by oschdi on 12/16/24.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <DeletionQueue.hpp>
#include <vector>
#include <VulkanContext.hpp>
#include <vulkan/vulkan_core.h>


class Pipeline {
public:
    Pipeline() { clear(); }
    Pipeline(std::shared_ptr<VulkanContext> context) : context(context) { clear(); }

    void build();
    void createShaderBindingTables(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

    void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkRayTracingShaderGroupTypeKHR shaderGroup);
    void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void addPushConstant(uint32_t size, VkShaderStageFlags shaderStage);
    void clear();
    void destroy();

    VkPipeline getHandle() const;
    VkPipelineLayout getLayoutHandle() const;
    uint32_t getGroupCount() const;

    AllocatedBuffer raygenShaderBindingTable;
    AllocatedBuffer missShaderBindingTable;
    AllocatedBuffer hitShaderBindingTable;

private:
    std::shared_ptr<VulkanContext> context;
    DeletionQueue deletionQueue;

    VkPipeline handle = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    std::vector<VkPushConstantRange> pushConstants{};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
};



#endif //PIPELINE_HPP
