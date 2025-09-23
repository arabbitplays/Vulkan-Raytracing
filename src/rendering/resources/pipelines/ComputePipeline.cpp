//
// Created by oschdi on 23.09.25.
//

#include "../../../../include/rendering/resources/pipelines/ComputePipeline.hpp"

namespace RtEngine {

    void ComputePipeline::build() {
        createPipelineLayout();

        VkComputePipelineCreateInfo info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.stage = shader_stages[0];
        info.layout = layout;
        if (vkCreateComputePipelines(context->device_manager->getDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &handle) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create ray tracing pipeline!");
        }

        deletionQueue.pushFunction([&]() {
           vkDestroyPipelineLayout(context->device_manager->getDevice(), layout, nullptr);
           vkDestroyPipeline(context->device_manager->getDevice(), handle, nullptr);
       });
    }

} // RtEngine