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

    pipeline->addPushConstant(MAX_PUSH_CONSTANT_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR);

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

    auto extract_views = [](std::vector<std::shared_ptr<Texture>> textures)
    {
        std::vector<VkImageView> imageViews{};
        for (auto& texture : textures)
        {
            imageViews.push_back(texture->image.imageView);
        }
        return imageViews;
    };

    descriptorAllocator.writeImages(1, extract_views(albedo_textures), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorAllocator.writeImages(2, extract_views(metal_rough_ao_textures), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    descriptorAllocator.writeImages(3, extract_views(normal_textures), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    descriptorAllocator.updateSet(context->device, materialDescriptorSet);
    descriptorAllocator.clearWrites();
}

std::shared_ptr<MetalRoughMaterial::MaterialResources> MetalRoughMaterial::createMaterialResources(MetalRoughParameters parameters)
{
    auto constants = std::make_shared<MaterialConstants>();
    constants->albedo = glm::vec4(parameters.albedo, 0.0f);
    constants->properties = glm::vec4(parameters.metallic, parameters.roughness, parameters.ao, parameters.eta); // TODO dynamic eta
    constants->emission = glm::vec4(parameters.emission_color, parameters.emission_power);

    auto resources = std::make_shared<MaterialResources>();
    resources->constants = constants;

    albedo_textures.push_back(default_tex);
    metal_rough_ao_textures.push_back(default_tex);
    normal_textures.push_back(default_normal_tex);

    return resources;
}

// is unique = true the method assumes that such an instance doesn't exist yet, so safe time when creating lots of instances,
// where it is clear that they are unique (used for loading scenes for example
std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(MetalRoughParameters parameters, bool unique) {
    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    std::shared_ptr<MaterialResources> resources = createMaterialResources(parameters);

    if (!unique)
    {
        for (uint32_t i = 0; i < resources_buffer.size(); i++)
        {
            if (*resources == *resources_buffer[i])
            {
                return instances[i];
            }
        }
    }

    resources_buffer.push_back(resources);
    instances.push_back(instance);

    return instance;
}

glm::vec4 MetalRoughMaterial::getEmissionForInstance(uint32_t material_instance_id) {
    return resources_buffer[material_instance_id]->constants->emission;
}

std::vector<std::shared_ptr<MetalRoughMaterial::MaterialResources>> MetalRoughMaterial::getResources()
{
    return resources_buffer;
}

void MetalRoughMaterial::initProperties()
{
    properties = std::make_shared<Properties>(MATERIAL_SECTION_NAME);
    properties->addBool("Normal_Mapping", &material_properties.normal_mapping);
    properties->addBool("Sample_Lights", &material_properties.sample_lights);
    properties->addBool("Sample_BSDF", &material_properties.sample_bsdf);
    properties->addBool("Russian_Roulette", &material_properties.russian_roulette);
}

AllocatedBuffer MetalRoughMaterial::createMaterialBuffer() {
    assert(resources_buffer.size() == instances.size());

    std::vector<MaterialConstants> materialConstants{};
    for (uint32_t i = 0; i < resources_buffer.size(); i++) {
        materialConstants.push_back(*resources_buffer[i]->constants);
        instances[i]->material_index = i;
    }
    return context->resource_builder->stageMemoryToNewBuffer(materialConstants.data(), materialConstants.size() * sizeof(MaterialConstants), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void MetalRoughMaterial::reset() {
    resources_buffer.clear();
    instances.clear();
    albedo_textures.clear();
    metal_rough_ao_textures.clear();
    normal_textures.clear();
    Material::reset();
}