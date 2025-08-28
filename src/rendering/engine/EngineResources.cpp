#include "EngineResources.hpp"

#include <spdlog/spdlog.h>

#include "DescriptorLayoutBuilder.hpp"

namespace RtEngine {
    EngineResources::EngineResources(const std::shared_ptr<VulkanContext> &vulkanContext) : vulkan_context(vulkanContext) {
        createEngineLayout();
        createEngineDescriptorSet(engine_descriptor_set_layout);
        createAndBindEngineBuffers();
    }

    void EngineResources::createEngineLayout() {
        DescriptorLayoutBuilder layoutBuilder;

        layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // denoising buffer
        layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // denoising hist buffer

        engine_descriptor_set_layout = layoutBuilder.build(
                vulkan_context->device_manager->getDevice(),
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);
        main_deletion_queue.pushFunction([&]() {
            vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), engine_descriptor_set_layout,
                                         nullptr);
        });
    }

    void EngineResources::createEngineDescriptorSet(const VkDescriptorSetLayout &layout) {
        engine_descriptor_set = vulkan_context->descriptor_allocator->allocate(
                vulkan_context->device_manager->getDevice(), layout);
    }

    void EngineResources::createAndBindEngineBuffers() {
        createEngineBuffers();
        bindEngineBuffers(engine_descriptor_set);
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

    void EngineResources::bindEngineBuffers(const VkDescriptorSet &descriptor_set) {
        VkDevice device = vulkan_context->device_manager->getDevice();

        uint32_t pixel_count = vulkan_context->swapchain->getPixelCount();
		vulkan_context->descriptor_allocator->writeBuffer(0, denoisingBuffer.handle, pixel_count * sizeof(SvgfData),
														  0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        vulkan_context->descriptor_allocator->writeBuffer(1, denoisingHistBuffer.handle, pixel_count * sizeof(SvgfHistData),
                                                          0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		vulkan_context->descriptor_allocator->updateSet(device, descriptor_set);
		vulkan_context->descriptor_allocator->clearWrites();
    }

    VkDescriptorSetLayout EngineResources::getEngineLayout() const {
        return engine_descriptor_set_layout;
    }

    VkDescriptorSet EngineResources::getEngineDescriptorSet() const {
        return engine_descriptor_set;
    }

    void EngineResources::destroy() {
        main_deletion_queue.flush();
        vulkan_context->resource_builder->destroyBuffer(denoisingBuffer);
        vulkan_context->resource_builder->destroyBuffer(denoisingHistBuffer);
    }
}
