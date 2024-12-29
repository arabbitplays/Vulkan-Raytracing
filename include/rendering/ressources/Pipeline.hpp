//
// Created by oschdi on 12/16/24.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <vector>
#include <vulkan/vulkan_core.h>


class Pipeline {
public:
    Pipeline() { clear(); }

    void build(VkDevice& device);
    void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkRayTracingShaderGroupTypeKHR shaderGroup);
    void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void addPushConstant(uint32_t size, VkShaderStageFlagBits shaderStage);
    void clear();
    void destroy(VkDevice device);

    VkPipeline getHandle() const;
    VkPipelineLayout getLayoutHandle() const;
    uint32_t getGroupCount() const;

private:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    std::vector<VkPushConstantRange> pushConstants{};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
};



#endif //PIPELINE_HPP
