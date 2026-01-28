#include "../../../../include/engine/rendering/renderer/ComputeRenderer.hpp"

#include <glitch.comp.spv.h>

#include "DescriptorLayoutBuilder.hpp"

namespace RtEngine {
    ComputeRenderer::ComputeRenderer(const std::shared_ptr<VulkanContext> &vulkan_context, const uint32_t max_frames_in_flight)
        : Renderer(vulkan_context, max_frames_in_flight) {
        createPipeline();
    }

    void ComputeRenderer::updateRenderTarget(const std::shared_ptr<RenderTarget> &target) {
        vulkan_context->descriptor_allocator->writeImage(0, target->getCurrentTargetImage().imageView, VK_NULL_HANDLE,
                                                         VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        vulkan_context->descriptor_allocator->updateSet(vulkan_context->device_manager->getDevice(), descriptor_set);
        vulkan_context->descriptor_allocator->clearWrites();
    }

    void ComputeRenderer::updateResources(const AllocatedImage &src_image) {
        vulkan_context->descriptor_allocator->writeImage(1, src_image.imageView, VK_NULL_HANDLE,
                                                                 VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        vulkan_context->descriptor_allocator->updateSet(vulkan_context->device_manager->getDevice(), descriptor_set);
        vulkan_context->descriptor_allocator->clearWrites();
    }

    void ComputeRenderer::createPipeline() {
        pipeline = std::make_shared<ComputePipeline>(vulkan_context);
        VkDevice device = vulkan_context->device_manager->getDevice();

        DescriptorLayoutBuilder layoutBuilder;
        initDescriptorLayout(layoutBuilder);
        descriptor_layout = layoutBuilder.build(device, VK_SHADER_STAGE_COMPUTE_BIT);
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
        layout_builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render target
        layout_builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // src image
    }

    void ComputeRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->getLayoutHandle(), 0, 1, &descriptor_set, 0, 0);

        VkExtent2D target_extent = target->getExtent();
        vkCmdDispatch(commandBuffer, (target_extent.width + 15) / 16, (target_extent.height + 15) / 16, 1);
    }

    void ComputeRenderer::submitCommandBuffer(VkCommandBuffer& command_buffer) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &command_buffer;

        if (vkQueueSubmit(vulkan_context->device_manager->getQueue(COMPUTE), 1, &submitInfo,
                          in_flight_fences[current_frame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

    void ComputeRenderer::cleanup() {
        deletion_queue.flush();
    }
} // RtEngine