#include "compute/MlmcRenderer.hpp"

#include "targets/RenderTargetKeys.hpp"
#include <combine.comp.spv.h>

namespace RtEngine {
    void MlmcRenderer::writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) {
        vulkan_context->descriptor_allocator->writeImage(0, target->getCurrRenderTargetImage(MLMC_TARGET_KEY).imageView, VK_NULL_HANDLE,
                                                         VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        vulkan_context->descriptor_allocator->writeImage(1, target->getCurrRenderTargetImage(MAIN_TARGET_KEY).imageView, VK_NULL_HANDLE,
                                                         VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        vulkan_context->descriptor_allocator->writeImage(2, target->getCurrRenderTargetImage(DIFF_TARGET_KEY).imageView, VK_NULL_HANDLE,
                                                         VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        vulkan_context->descriptor_allocator->updateSet(vulkan_context->device_manager->getDevice(), descriptor_set);
        vulkan_context->descriptor_allocator->clearWrites();
    }

    void MlmcRenderer::writeResources(const std::shared_ptr<DrawContext> &draw_context,
        UpdateFlagsHandle update_flags) {
    }


    void MlmcRenderer::initDescriptorLayout(DescriptorLayoutBuilder& layout_builder) {
        layout_builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render target
        layout_builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // biased image
        layout_builder.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // diff image
    }

    void MlmcRenderer::recordDispatch(VkCommandBuffer command_buffer, std::shared_ptr<RenderTargetRepository> &target) {
        VkExtent2D target_extent = target->getExtent();
        vkCmdDispatch(command_buffer, (target_extent.width + 15) / 16, (target_extent.height + 15) / 16, 1);
    }


    VkShaderModule MlmcRenderer::createShaderModule() {
        return VulkanUtil::createShaderModule(
            vulkan_context->device_manager->getDevice(), oschd_combine_comp_spv_size(), oschd_combine_comp_spv());
    }

}
