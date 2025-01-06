//
// Created by oschdi on 12/30/24.
//

#include "MetalRoughMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <miss.rmiss.spv.h>
#include <OptionsWindow.hpp>
#include <raygen.rgen.spv.h>
#include <shadow_miss.rmiss.spv.h>
#include <metal_rough_closesthit.rchit.spv.h>
#include <VulkanUtil.hpp>

void MetalRoughMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
    DescriptorLayoutBuilder layoutBuilder;
    pipeline = std::make_shared<Pipeline>(context);

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64);

    materialLayout = layoutBuilder.build(context->device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device, materialLayout, nullptr);
    });

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
    pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

    pipeline->addPushConstant(sizeof(RaytracingOptions), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

    VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(context->device, oschd_raygen_rgen_spv_size(), oschd_raygen_rgen_spv());
    VkShaderModule missShaderModule = VulkanUtil::createShaderModule(context->device, oschd_miss_rmiss_spv_size(), oschd_miss_rmiss_spv());
    VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(context->device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
    VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(context->device, oschd_metal_rough_closesthit_rchit_spv_size(), oschd_metal_rough_closesthit_rchit_spv());

    pipeline->addShaderStage(raygenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(shadowMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(closestHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);

    pipeline->build();

    mainDeletionQueue.pushFunction([&]() {
        pipeline->destroy();
    });

    vkDestroyShaderModule(context->device, raygenShaderModule, nullptr);
    vkDestroyShaderModule(context->device, missShaderModule, nullptr);
    vkDestroyShaderModule(context->device, shadowMissShaderModule, nullptr);
    vkDestroyShaderModule(context->device, closestHitShaderModule, nullptr);
}

void MetalRoughMaterial::writeMaterial() {
    materialBuffer = createMaterialBuffer();
    resetQueue.pushFunction([&]() {
        context->resource_builder->destroyBuffer(materialBuffer);
    });

    materialDescriptorSet = descriptorAllocator.allocate(context->device, materialLayout);

    descriptorAllocator.writeBuffer(0, materialBuffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    std::vector<VkImageView> albedo_views{};
    for (auto& image : albedo_textures) {
        albedo_views.push_back(image.imageView);
    }
    descriptorAllocator.writeImages(1, albedo_views, sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    std::vector<VkImageView> metal_rough_views{};
    for (auto& image : metal_rough_ao_textures) {
        metal_rough_views.push_back(image.imageView);
    }
    descriptorAllocator.writeImages(2, metal_rough_views, sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    descriptorAllocator.updateSet(context->device, materialDescriptorSet);
    descriptorAllocator.clearWrites();
}

std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(glm::vec3 albedo, float metallic, float roughness, float ao) {
    return createInstance(albedo, default_tex, metallic, roughness, ao, default_tex);
}

std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(const AllocatedImage &albedo_tex, const AllocatedImage &metal_rough_ao_tex) {
    return createInstance(glm::vec3(0), albedo_tex, 0, 0, 0, metal_rough_ao_tex);
}

std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(glm::vec3 albedo, const AllocatedImage &albedo_tex, float metallic, float roughness, float ao, const AllocatedImage &metal_rough_ao_tex) {
    auto constants = std::make_shared<MaterialConstants>();
    constants->albedo = glm::vec4(albedo, 0.0f);
    constants->properties = glm::vec4(metallic, roughness, ao, 0.0f);

    auto resources = std::make_shared<MaterialRessources>();
    resources->constants = constants;

    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    instances.push_back(instance);
    constants_buffer.push_back(resources->constants);
    albedo_textures.push_back(albedo_tex);
    metal_rough_ao_textures.push_back(metal_rough_ao_tex);
    return instance;
}

AllocatedBuffer MetalRoughMaterial::createMaterialBuffer() {
    assert(constants_buffer.size() == instances.size());

    std::vector<MaterialConstants> materialConstants{};
    for (uint32_t i = 0; i < constants_buffer.size(); i++) {
        instances[i]->material_index = i;
        materialConstants.push_back(*constants_buffer[i]);
    }
    return context->resource_builder->stageMemoryToNewBuffer(materialConstants.data(), materialConstants.size() * sizeof(MaterialConstants), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void MetalRoughMaterial::reset() {
    constants_buffer.clear();
    albedo_textures.clear();
    metal_rough_ao_textures.clear();
    instances.clear();
    Material::reset();
}