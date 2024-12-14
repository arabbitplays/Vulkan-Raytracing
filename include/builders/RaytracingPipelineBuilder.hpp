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
    RaytracingPipelineBuilder() = default;

    void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkRayTracingShaderGroupTypeKHR shaderGroup);
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};

    private:
};



#endif //RAYTRACINGPIPELINEBUILDER_HPP
