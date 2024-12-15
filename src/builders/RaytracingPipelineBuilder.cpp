//
// Created by oschdi on 12/14/24.
//

#include "RaytracingPipelineBuilder.hpp"

#include <stdexcept>

VkResult CreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {

    auto func = (PFN_vkCreateRayTracingPipelinesKHR) vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    if (func != nullptr) {
        return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void RaytracingPipelineBuilder::buildPipeline(VkDevice& device, VkPipeline* pipeline, VkPipelineLayout* layout) {
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
    pipelineInfo.pStages = shader_stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(shader_groups.size());
    pipelineInfo.pGroups = shader_groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout = *layout;
    if (CreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }
}

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
    shader_groups.push_back(shaderGroupInfo);
}

void RaytracingPipelineBuilder::setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) {
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
}

void RaytracingPipelineBuilder::clear() {
    pipelineLayoutInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    shader_stages.clear();
    shader_groups.clear();
}

void RaytracingPipelineBuilder::destroyPipeline(VkDevice device, VkPipeline& pipeline, VkPipelineLayout& layout) {
    vkDestroyPipelineLayout(device, layout, nullptr);
}
