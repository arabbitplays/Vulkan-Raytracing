#include "../../../include/engine/renderer/ComputeRenderer.hpp"

#include <glitch.comp.spv.h>

#include "DescriptorLayoutBuilder.hpp"

namespace RtEngine {
    ComputeRenderer::ComputeRenderer(const std::shared_ptr<VulkanContext> &vulkan_context) : vulkan_context(vulkan_context) {
        createPipeline();
    }

    void ComputeRenderer::createPipeline() {
        pipeline = std::make_shared<ComputePipeline>(vulkan_context);
        VkDevice device = vulkan_context->device_manager->getDevice();

        DescriptorLayoutBuilder layoutBuilder;
        initDescriptorLayout(layoutBuilder);
        descriptor_layout = layoutBuilder.build(device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        deletion_queue.pushFunction([&]() {
            vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout, nullptr);
        });
        descriptor_set = vulkan_context->descriptor_allocator->allocate(vulkan_context->device_manager->getDevice(), descriptor_layout);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{descriptor_layout};
        pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

        VkShaderModule compute_shader_module = VulkanUtil::createShaderModule(
                device, oschd_glitch_comp_spv_size(), oschd_glitch_comp_spv());
        pipeline->setShaderStage(compute_shader_module);

        pipeline->build();

        deletion_queue.pushFunction([&]() { pipeline->destroy(); });

        vkDestroyShaderModule(device, compute_shader_module, nullptr);
    }

    void ComputeRenderer::initDescriptorLayout(DescriptorLayoutBuilder& layout_builder) {
        layout_builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render image
    }

    void ComputeRenderer::cleanup() {
        deletion_queue.flush();
    }
} // RtEngine