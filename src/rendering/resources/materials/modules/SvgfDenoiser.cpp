//
// Created by oschdi on 23.09.25.
//

#include "../../../../../include/rendering/resources/materials/modules/SvgfDenoiser.hpp"

#include <svgf_filter.comp.spv.h>

#include "CommandBufferUtil.hpp"
#include "DescriptorLayoutBuilder.hpp"
#include "GBuffer.hpp"
#include "imstb_truetype.h"

namespace RtEngine {
    SvgfDenoiser::SvgfDenoiser(const std::shared_ptr<VulkanContext> &context) : context(context) {
        initLayout();
        deletion_queue.pushFunction([&]() {
           destroyLayout();
        });
    }

    void SvgfDenoiser::createComputePipeline() {
        compute_pipeline = std::make_shared<ComputePipeline>(context);

        std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};
        descriptor_set_layouts.push_back(getDescriptorLayout());
        compute_pipeline->setDescriptorSetLayouts(descriptor_set_layouts);

        VkShaderModule compute_shader_module = VulkanUtil::createShaderModule(
            context->device_manager->getDevice(), oschd_svgf_filter_comp_spv_size(), oschd_svgf_filter_comp_spv());
        compute_pipeline->addShaderStage(compute_shader_module, VK_SHADER_STAGE_COMPUTE_BIT);

        compute_pipeline->build();

        deletion_queue.pushFunction([&]() {
           compute_pipeline->destroy();
        });

        vkDestroyShaderModule(context->device_manager->getDevice(), compute_shader_module, nullptr);

    }

    void SvgfDenoiser::writeResources(const AllocatedImage &target_image, const std::shared_ptr<GBuffer> &g_buffer) const {
        context->descriptor_allocator->writeImage(0, target_image.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        context->descriptor_allocator->writeBuffer(1, g_buffer->getBuffer().handle, g_buffer->getBufferSize(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        context->descriptor_allocator->writeBuffer(2, g_buffer->getHistBuffer().handle, g_buffer->getHistBufferSize(), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        context->descriptor_allocator->updateSet(descriptor_set->getCurrentSet());
        context->descriptor_allocator->clearWrites();
    }

    void SvgfDenoiser::recordCommands(VkCommandBuffer command_buffer) {
        CommandBufferUtil::recordMemoryBarrier(command_buffer,
            VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline->getHandle());

        std::vector<VkDescriptorSet> descriptor_sets{};
        descriptor_sets.push_back(descriptor_set->getCurrentSet());
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline->getLayoutHandle(), 0,
            descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

        VkExtent2D image_extent = context->swapchain->extent;
        glm::ivec2 workgroup_count = glm::ivec2((image_extent.width + 15) / 16, (image_extent.height + 15) / 16);
        vkCmdDispatch(command_buffer, workgroup_count.x, workgroup_count.y, 1);
    }

    VkDescriptorSetLayout SvgfDenoiser::createLayout() {
        DescriptorLayoutBuilder layout_builder;

        layout_builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); // render image
        layout_builder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // g buffer
        layout_builder.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // hist g buffer

        return layout_builder.build(context->device_manager->getDevice(), VK_SHADER_STAGE_COMPUTE_BIT);
    }

    std::shared_ptr<DescriptorSet> SvgfDenoiser::createDescriptorSet(const VkDescriptorSetLayout &layout) {
        return std::make_shared<DescriptorSet>(context->descriptor_allocator, layout);
    }

    void SvgfDenoiser::destroyLayout() {
        vkDestroyDescriptorSetLayout(context->device_manager->getDevice(), descriptor_layout,
                         nullptr);
    }

    void SvgfDenoiser::destroyResources() {
        deletion_queue.flush();
    }
}
