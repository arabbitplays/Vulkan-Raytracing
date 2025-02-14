//
// Created by oschdi on 12/30/24.
//

#include "MetalRoughMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <miss.rmiss.spv.h>
#include <OptionsWindow.hpp>
#include <shadow_miss.rmiss.spv.h>
#include <metal_rough_closesthit.rchit.spv.h>
#include <metal_rough_raygen.rgen.spv.h>
#include <VulkanUtil.hpp>

void MetalRoughMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
    DescriptorLayoutBuilder layoutBuilder;
    pipeline = std::make_shared<Pipeline>(context);

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256);
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256);

    materialLayout = layoutBuilder.build(context->device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device, materialLayout, nullptr);
    });

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
    pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

    pipeline->addPushConstant(sizeof(RaytracingOptions), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(context->device, oschd_metal_rough_raygen_rgen_spv_size(), oschd_metal_rough_raygen_rgen_spv());
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


    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    default_tex = std::make_shared<Texture>("", PARAMETER, "", context->resource_builder->createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t blue = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1, 0));
    default_normal_tex = std::make_shared<Texture>("", NORMAL, "", context->resource_builder->createImage((void*)&blue, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT));

    mainDeletionQueue.pushFunction([&]() {
        context->resource_builder->destroyImage(default_tex->image);
        context->resource_builder->destroyImage(default_normal_tex->image);
    });
}

void MetalRoughMaterial::writeMaterial() {
    materialBuffer = createMaterialBuffer();
    resetQueue.pushFunction([&]() {
        context->resource_builder->destroyBuffer(materialBuffer);
    });

    materialDescriptorSet = descriptorAllocator.allocate(context->device, materialLayout);

    descriptorAllocator.writeBuffer(0, materialBuffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    std::vector<VkImageView> albedo_views{};
    std::vector<VkImageView> metal_rough_views{};
    std::vector<VkImageView> normal_views{};
    for (auto& resources : resources_buffer)
    {
        albedo_views.push_back(resources->albedo_tex.image.imageView);
        metal_rough_views.push_back(resources->metal_rough_ao_tex.image.imageView);
        normal_views.push_back(resources->normal_tex.image.imageView);
    }
    descriptorAllocator.writeImages(1, albedo_views, sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorAllocator.writeImages(2, metal_rough_views, sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorAllocator.writeImages(3, normal_views, sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    descriptorAllocator.updateSet(context->device, materialDescriptorSet);
    descriptorAllocator.clearWrites();
}

std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(MetalRoughParameters parameters) {
    if (parameters.albedo_tex == nullptr) {
        parameters.albedo_tex = default_tex;
    }

    if (parameters.metal_rough_ao_tex == nullptr) {
        parameters.metal_rough_ao_tex = default_tex;
    }

    if (parameters.normal_tex == nullptr) {
        parameters.normal_tex = default_normal_tex;
    }

    auto constants = std::make_shared<MaterialConstants>();
    constants->albedo = glm::vec4(parameters.albedo, 0.0f);
    constants->properties = glm::vec4(parameters.metallic, parameters.roughness, parameters.ao, 0.0f);
    constants->emission = glm::vec4(parameters.emission_color, parameters.emission_power);

    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    instances.push_back(instance);
    auto resources = std::make_shared<MaterialResources>();
    resources->constants = constants;
    resources->albedo_tex = *parameters.albedo_tex;
    resources->metal_rough_ao_tex = *parameters.metal_rough_ao_tex;
    resources->normal_tex = *parameters.normal_tex;
    resources_buffer.push_back(resources);

    return instance;
}

glm::vec4 MetalRoughMaterial::getEmissionForInstance(uint32_t material_instance_id) {
    return resources_buffer[material_instance_id]->constants->emission;
}

std::shared_ptr<MetalRoughMaterial::MaterialResources> MetalRoughMaterial::getResourcesForInstance(uint32_t material_instance_id)
{
    return resources_buffer[material_instance_id];
}

AllocatedBuffer MetalRoughMaterial::createMaterialBuffer() {
    assert(resources_buffer.size() == instances.size());

    std::vector<MaterialConstants> materialConstants{};
    for (uint32_t i = 0; i < resources_buffer.size(); i++) {
        instances[i]->material_index = i;
        materialConstants.push_back(*resources_buffer[i]->constants);
    }
    return context->resource_builder->stageMemoryToNewBuffer(materialConstants.data(), materialConstants.size() * sizeof(MaterialConstants), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void MetalRoughMaterial::reset() {
    resources_buffer.clear();
    instances.clear();
    Material::reset();
}