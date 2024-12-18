//
// Created by oschdi on 12/17/24.
//

#include "PhongMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <VulkanUtil.hpp>
#include <glm/detail/type_mat4x3.hpp>

#include "miss.rmiss.spv.h"
#include "shadow_miss.rmiss.spv.h"
#include "closesthit.rchit.spv.h"
#include "raygen.rgen.spv.h"

void PhongMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
    DescriptorLayoutBuilder layoutBuilder;
    pipeline = std::make_shared<Pipeline>();

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    deletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
    });

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
    pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

    VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(device, oschd_raygen_rgen_spv_size(), oschd_raygen_rgen_spv());
    VkShaderModule missShaderModule = VulkanUtil::createShaderModule(device, oschd_miss_rmiss_spv_size(), oschd_miss_rmiss_spv());
    VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
    VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(device, oschd_closesthit_rchit_spv_size(), oschd_closesthit_rchit_spv());

    pipeline->addShaderStage(raygenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(shadowMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    pipeline->addShaderStage(closestHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);

    pipeline->build(device);

    deletionQueue.pushFunction([&]() {
        pipeline->destroy(device);
    });

    vkDestroyShaderModule(device, raygenShaderModule, nullptr);
    vkDestroyShaderModule(device, missShaderModule, nullptr);
    vkDestroyShaderModule(device, shadowMissShaderModule, nullptr);
    vkDestroyShaderModule(device, closestHitShaderModule, nullptr);
}

void PhongMaterial::writeMaterial() {
    materialBuffer = createMaterialBuffer();
    deletionQueue.pushFunction([&]() {
        ressource_builder.destroyBuffer(materialBuffer);
    });

    materialDescriptorSet = descriptorAllocator.allocate(device, materialLayout);
    descriptorAllocator.writeBuffer(0, materialBuffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.updateSet(device, materialDescriptorSet);
    descriptorAllocator.clearWrites();
}

std::shared_ptr<MaterialInstance> PhongMaterial::createInstance(
        glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient,
        glm::vec3 reflection, glm::vec3 transmission,
        float n, glm::vec3 eta) {
    auto constants = std::make_shared<PhongMaterial::MaterialConstants>();
    constants->diffuse = diffuse;
    constants->specular = specular;
    constants->ambient = ambient;
    constants->reflection = reflection;
    constants->transmission = transmission;
    constants->n = n;
    constants-> eta = glm::vec4(eta, 0.0f);

    auto resources = std::make_shared<PhongMaterial::MaterialRessources>();
    resources->constants = constants;

    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    instances.push_back(instance);
    constants_buffer.push_back(resources->constants);
    return instance;
}

AllocatedBuffer PhongMaterial::createMaterialBuffer() {
    assert(constants_buffer.size() == instances.size());

    std::vector<MaterialConstants> materialConstants{};
    for (uint32_t i = 0; i < constants_buffer.size(); i++) {
        instances[i]->material_index = i;
        materialConstants.push_back(*constants_buffer[i]);
    }
    return ressource_builder.stageMemoryToNewBuffer(materialConstants.data(), materialConstants.size() * sizeof(MaterialConstants), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

