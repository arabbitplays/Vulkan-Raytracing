//
// Created by oschdi on 12/14/24.
//

#ifndef RAYTRACINGPIPELINEBUILDER_HPP
#define RAYTRACINGPIPELINEBUILDER_HPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>


class RaytracingPipelineBuilder {
  public:
    RaytracingPipelineBuilder() { clear(); }

    void buildPipeline(VkDevice& device, VkPipeline* pipeline, VkPipelineLayout* layout);
    void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkRayTracingShaderGroupTypeKHR shaderGroup);
    void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void clear();
    void destroyPipeline(VkDevice device, VkPipeline &pipeline, VkPipelineLayout &layout);

private:
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};

};



#endif //RAYTRACINGPIPELINEBUILDER_HPP
