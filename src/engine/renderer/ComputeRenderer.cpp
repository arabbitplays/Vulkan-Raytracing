#include "../../../include/engine/renderer/ComputeRenderer.hpp"

#include <glitch.comp.spv.h>

namespace RtEngine {
    ComputeRenderer::ComputeRenderer(const std::shared_ptr<VulkanContext> &vulkan_context) : vulkan_context(vulkan_context) {
        createPipeline();
    }

    void ComputeRenderer::createPipeline() {
        pipeline = std::make_shared<ComputePipeline>(vulkan_context);
        VkDevice device = vulkan_context->device_manager->getDevice();

        VkShaderModule compute_shader_module = VulkanUtil::createShaderModule(
                device, oschd_glitch_comp_spv_size(), oschd_glitch_comp_spv());
        pipeline->setShaderStage(compute_shader_module);

        pipeline->build();

        deletion_queue.pushFunction([&]() { pipeline->destroy(); });

        vkDestroyShaderModule(device, compute_shader_module, nullptr);
    }

    void ComputeRenderer::cleanup() {
        deletion_queue.flush();
    }
} // RtEngine