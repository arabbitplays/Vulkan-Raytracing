//
// Created by oschdi on 23.09.25.
//

#include "../../../../../include/rendering/resources/materials/modules/SvgfDenoiser.hpp"

#include <svgf_filter.comp.spv.h>

namespace RtEngine {
    SvgfDenoiser::SvgfDenoiser(const std::shared_ptr<VulkanContext> &context) : context(context) { }

    void SvgfDenoiser::createComputePipeline(std::vector<VkDescriptorSetLayout> layouts) {
        compute_pipeline = std::make_shared<ComputePipeline>(context);

        compute_pipeline->setDescriptorSetLayouts(layouts);

        VkShaderModule compute_shader_module = VulkanUtil::createShaderModule(
            context->device_manager->getDevice(), oschd_svgf_filter_comp_spv_size(), oschd_svgf_filter_comp_spv());
        compute_pipeline->addShaderStage(compute_shader_module, VK_SHADER_STAGE_COMPUTE_BIT);

        compute_pipeline->build();

        vkDestroyShaderModule(context->device_manager->getDevice(), compute_shader_module, nullptr);

    }

}
