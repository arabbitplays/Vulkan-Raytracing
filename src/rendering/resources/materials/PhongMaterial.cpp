#include "PhongMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>
#include <glm/detail/type_mat4x3.hpp>

#include <environment_miss.rmiss.spv.h>
#include <phong_closesthit.rchit.spv.h>
#include <phong_raygen.rgen.spv.h>
#include "miss.rmiss.spv.h"
#include "PhongInstance.hpp"
#include "shadow_miss.rmiss.spv.h"

namespace RtEngine {
	void PhongMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
		DescriptorLayoutBuilder layoutBuilder;
		pipeline = std::make_shared<Pipeline>(vulkan_context);
		VkDevice device = vulkan_context->device_manager->getDevice();

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		mainDeletionQueue.pushFunction([&]() {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), materialLayout, nullptr);
		});

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
		pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

		pipeline->addPushConstant(MAX_PUSH_CONSTANT_SIZE,
								  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR);

		VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(device, oschd_phong_raygen_rgen_spv_size(),
																		   oschd_phong_raygen_rgen_spv());
		VkShaderModule missShaderModule = VulkanUtil::createShaderModule(
				device, oschd_environment_miss_rmiss_spv_size(), oschd_environment_miss_rmiss_spv());
		VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(
				device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
		VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(
				device, oschd_phong_closesthit_rchit_spv_size(), oschd_phong_closesthit_rchit_spv());

		pipeline->addShaderStage(raygenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(shadowMissShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
		pipeline->addShaderStage(closestHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
								 VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);

		pipeline->build();

		mainDeletionQueue.pushFunction([&]() { pipeline->destroy(); });

		vkDestroyShaderModule(device, raygenShaderModule, nullptr);
		vkDestroyShaderModule(device, missShaderModule, nullptr);
		vkDestroyShaderModule(device, shadowMissShaderModule, nullptr);
		vkDestroyShaderModule(device, closestHitShaderModule, nullptr);
	}

	void PhongMaterial::writeMaterial() {
		if (material_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(material_buffer);
		}
		material_buffer = createMaterialBuffer();

		VkDevice device = vulkan_context->device_manager->getDevice();
		materialDescriptorSet = descriptorAllocator.allocate(device, materialLayout);
		descriptorAllocator.writeBuffer(0, material_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		descriptorAllocator.updateSet(device, materialDescriptorSet);
		descriptorAllocator.clearWrites();
	}

	std::shared_ptr<MaterialInstance> PhongMaterial::loadInstance(const YAML::Node& yaml_node) {
		std::shared_ptr<MaterialInstance> instance = std::make_shared<PhongInstance>();
		instance->loadResources(yaml_node);
		instances.push_back(instance);
		return instance;
	}

	AllocatedBuffer PhongMaterial::createMaterialBuffer() {
		std::vector<void*> resource_ptrs(instances.size());
		std::vector<size_t> sizes(instances.size());
		size_t total_size = 0;
		for (uint32_t i = 0; i < instances.size(); i++) {
			resource_ptrs[i] = instances[i]->getResources(&sizes[i]);
			instances[i]->setMaterialIndex(i);
			total_size += sizes[i];
		}

		const auto material_data = static_cast<std::byte*>(std::malloc(total_size));
		std::byte* dst = material_data;
		for (uint32_t i = 0; i < resource_ptrs.size(); i++) {
			std::memcpy(dst, resource_ptrs[i], sizes[i]);
			dst += sizes[i];
		}

		AllocatedBuffer material_buffer = vulkan_context->resource_builder->stageMemoryToNewBuffer(
				material_data, total_size,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		free(material_data);
		return material_buffer;
	}

	void PhongMaterial::initProperties() {
		properties = std::make_shared<PropertiesSection>(MATERIAL_SECTION_NAME);
		properties->addBool("Shadows", &material_properties.shadows);
		properties->addBool("Fresnel", &material_properties.fresnel);
		properties->addBool("Dispersion", &material_properties.dispersion);
	}

	std::vector<std::shared_ptr<Texture>> PhongMaterial::getTextures() { return {}; }

	void PhongMaterial::reset() {
		instances.clear();
		Material::reset();
	}
} // namespace RtEngine
