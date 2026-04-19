#include "compute/GlitchRenderer.hpp"

#include <filesystem>
#include <glitch.comp.spv.h>

#include "targets/RenderTargetKeys.hpp"

namespace RtEngine {
    GlitchRenderer::GlitchRenderer(const std::shared_ptr<VulkanContext> &vulkan_context,
                                   const uint32_t max_frames_in_flight) : ComputeRenderer(
        vulkan_context, max_frames_in_flight) {
    }

    void GlitchRenderer::writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) {
        vulkan_context->descriptor_allocator->writeImage(0, target->getCurrRenderTargetImage(MAIN_TARGET_KEY).imageView, VK_NULL_HANDLE,
                                                         VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        vulkan_context->descriptor_allocator->updateSet(vulkan_context->device_manager->getDevice(), descriptor_set);
        vulkan_context->descriptor_allocator->clearWrites();
    }

    void GlitchRenderer::writeResources(const std::shared_ptr<DrawContext> &draw_context,
        UpdateFlagsHandle update_flags) {
        // TODO find an abstract way to put stuff like this into a compute renderer
        for (const auto& entry : std::filesystem::directory_iterator(INPUT_DIR)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::filesystem::path file_name = entry.path().filename();
            if (file_name.extension() != ".png") {
                continue;
            }

            AllocatedImage input_img = vulkan_context->resource_builder->loadImage(entry.path().string(), VK_IMAGE_LAYOUT_GENERAL);
            vulkan_context->descriptor_allocator->writeImage(1, input_img.imageView, VK_NULL_HANDLE,
                                                                     VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

            vulkan_context->descriptor_allocator->updateSet(vulkan_context->device_manager->getDevice(), descriptor_set);
            vulkan_context->descriptor_allocator->clearWrites();

            break;
        }
    }


    void GlitchRenderer::initDescriptorLayout(DescriptorLayoutBuilder& layout_builder) {
        layout_builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render target
        layout_builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // src image
    }

    void GlitchRenderer::recordDispatch(VkCommandBuffer command_buffer, std::shared_ptr<RenderTargetRepository> &target) {
        VkExtent2D target_extent = target->getExtent();
        vkCmdDispatch(command_buffer, 1, (target_extent.height + 255) / 256, 1);
    }


    VkShaderModule GlitchRenderer::createShaderModule() {
        return VulkanUtil::createShaderModule(
            vulkan_context->device_manager->getDevice(), oschd_glitch_comp_spv_size(), oschd_glitch_comp_spv());
    }

}
