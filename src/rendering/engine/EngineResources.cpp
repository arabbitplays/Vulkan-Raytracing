#include "EngineResources.hpp"

#include <spdlog/spdlog.h>

#include "DescriptorLayoutBuilder.hpp"

namespace RtEngine {
    EngineResources::EngineResources(const std::shared_ptr<VulkanContext> &vulkanContext) : vulkan_context(vulkanContext) {
        initLayout();
        createAndBindResources();
    }

    VkDescriptorSetLayout EngineResources::createLayout() {
        DescriptorLayoutBuilder layoutBuilder;

        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // denoising buffer
        layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // denoising hist buffer

        return layoutBuilder.build(
                vulkan_context->device_manager->getDevice(),
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);

    }

    std::shared_ptr<DescriptorSet> EngineResources::createDescriptorSet(const VkDescriptorSetLayout &layout) {
        return std::make_shared<DescriptorSet>(vulkan_context->descriptor_allocator, vulkan_context->device_manager, layout, 1); // TODO 1 may not be right here, but the denoising buffers need to stay consistent so!?!?
    }

    void EngineResources::createAndBindResources() {
        createEngineBuffers();
        bindEngineBuffers(descriptor_set->getCurrentSet());
    }

    void EngineResources::destroyResources() {
        destroyLayout();
        vulkan_context->resource_builder->destroyBuffer(denoisingBuffer);
        vulkan_context->resource_builder->destroyBuffer(denoisingHistBuffer);
    }

    void EngineResources::createEngineBuffers() {
        if (denoisingBuffer.handle != VK_NULL_HANDLE) {
            vulkan_context->resource_builder->destroyBuffer(denoisingBuffer);
            vulkan_context->resource_builder->destroyBuffer(denoisingHistBuffer);
        }

        uint32_t pixel_count = vulkan_context->swapchain->getPixelCount();
        denoisingBuffer = vulkan_context->resource_builder->createZeroBuffer(pixel_count * sizeof(SvgfData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        denoisingHistBuffer = vulkan_context->resource_builder->createZeroBuffer(pixel_count * sizeof(SvgfHistData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    void EngineResources::bindEngineBuffers(const VkDescriptorSet &descriptor_set) const {
        VkDevice device = vulkan_context->device_manager->getDevice();

        uint32_t pixel_count = vulkan_context->swapchain->getPixelCount();
		vulkan_context->descriptor_allocator->writeBuffer(0, denoisingBuffer.handle, pixel_count * sizeof(SvgfData),
														  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vulkan_context->descriptor_allocator->writeBuffer(1, denoisingHistBuffer.handle, pixel_count * sizeof(SvgfHistData),
                                                          0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		vulkan_context->descriptor_allocator->updateSet(device, descriptor_set);
		vulkan_context->descriptor_allocator->clearWrites();
    }

    void EngineResources::destroyLayout() {
        vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout,
                                         nullptr);
    }
}
