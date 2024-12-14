//
// Created by oschdi on 12/14/24.
//

#include "RaytracingPipelineBuilder.hpp"


void RaytracingPipelineBuilder::addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage, VkRayTracingShaderGroupTypeKHR shaderGroup) {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = shaderStage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";
    shader_stages.push_back(shaderStageInfo);

    VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
    shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupInfo.type = shaderGroup; // TODO what are the alternatives?
    shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    if (shaderGroup == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR) {
        shaderGroupInfo.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
    } else if (shaderGroup == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR) {
        shaderGroupInfo.closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 1;

    }
    shaderGroups.push_back(shaderGroupInfo);
}
