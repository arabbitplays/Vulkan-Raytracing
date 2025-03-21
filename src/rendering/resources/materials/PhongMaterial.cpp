//
// Created by oschdi on 12/17/24.
//

#include "PhongMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>
#include <glm/detail/type_mat4x3.hpp>

#include "miss.rmiss.spv.h"
#include "shadow_miss.rmiss.spv.h"
#include <phong_raygen.rgen.spv.h>
#include <phong_closesthit.rchit.spv.h>
#include <environment_miss.rmiss.spv.h>

void PhongMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
    DescriptorLayoutBuilder layoutBuilder;
    pipeline = std::make_shared<Pipeline>(context);

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    materialLayout = layoutBuilder.build(context->device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(context->device, materialLayout, nullptr);
    });

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
    pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

    pipeline->addPushConstant(MAX_PUSH_CONSTANT_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR);

    VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(context->device, oschd_phong_raygen_rgen_spv_size(), oschd_phong_raygen_rgen_spv());
    VkShaderModule missShaderModule = VulkanUtil::createShaderModule(context->device, oschd_environment_miss_rmiss_spv_size(), oschd_environment_miss_rmiss_spv());
    VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(context->device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
    VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(context->device, oschd_phong_closesthit_rchit_spv_size(), oschd_phong_closesthit_rchit_spv());

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

void PhongMaterial::writeMaterial() {
    materialBuffer = createMaterialBuffer();
    resetQueue.pushFunction([&]() {
        context->resource_builder->destroyBuffer(materialBuffer);
    });

    materialDescriptorSet = descriptorAllocator.allocate(context->device, materialLayout);
    descriptorAllocator.writeBuffer(0, materialBuffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.updateSet(context->device, materialDescriptorSet);
    descriptorAllocator.clearWrites();
}

std::shared_ptr<MaterialInstance> PhongMaterial::createInstance(
        glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient,
        glm::vec3 reflection, glm::vec3 transmission,
        float n, glm::vec3 eta) {
    auto constants = std::make_shared<PhongMaterial::PhongMaterialConstants>();
    constants->diffuse = diffuse;
    constants->specular = specular;
    constants->ambient = ambient;
    constants->reflection = reflection;
    constants->transmission = transmission;
    constants->n = n;
    constants-> eta = glm::vec4(eta, 0.0f);

    auto resources = std::make_shared<PhongMaterial::MaterialResources>();
    resources->constants = constants;

    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    instances.push_back(instance);
    resources_buffer.push_back(resources);
    return instance;
}

AllocatedBuffer PhongMaterial::createMaterialBuffer() {
    assert(resources_buffer.size() == instances.size());

    std::vector<PhongMaterialConstants> materialConstants{};
    for (uint32_t i = 0; i < resources_buffer.size(); i++) {
        instances[i]->material_index = i;
        materialConstants.push_back(*resources_buffer[i]->constants);
    }
    return context->resource_builder->stageMemoryToNewBuffer(materialConstants.data(), materialConstants.size() * sizeof(PhongMaterialConstants), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

std::vector<std::shared_ptr<PhongMaterial::MaterialResources>> PhongMaterial::getResources()
{
    return resources_buffer;
}

void PhongMaterial::initProperties()
{
    properties = std::make_shared<Properties>(MATERIAL_SECTION_NAME);
    properties->addBool("Shadows", &material_properties.shadows);
    properties->addBool("Fresnel", &material_properties.fresnel);
    properties->addBool("Dispersion", &material_properties.dispersion);
}


void PhongMaterial::reset() {
    resources_buffer.clear();
    instances.clear();
    Material::reset();
}

