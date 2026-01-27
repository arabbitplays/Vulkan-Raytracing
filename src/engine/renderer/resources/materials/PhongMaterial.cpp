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
		pipeline = std::make_shared<RaytracingPipeline>(vulkan_context);
		VkDevice device = vulkan_context->device_manager->getDevice();

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		mainDeletionQueue.pushFunction([&]() {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), materialLayout, nullptr);
		});
		materialDescriptorSet = descriptorAllocator.allocate(device, materialLayout);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
		pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

		pipeline->addPushConstant( 16 * sizeof(uint32_t),
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

	void PhongMaterial::writeMaterial(AllocatedBuffer material_buffer, std::shared_ptr<MaterialTextures<>> material_textures) {
		VkDevice device = vulkan_context->device_manager->getDevice();
		descriptorAllocator.writeBuffer(0, material_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		descriptorAllocator.updateSet(device, materialDescriptorSet);
		descriptorAllocator.clearWrites();
	}

	std::shared_ptr<MaterialInstance> PhongMaterial::loadInstance(const YAML::Node& yaml_node) {
		std::shared_ptr<MaterialInstance> instance = std::make_shared<PhongInstance>("");
		instance->loadResources(yaml_node);
		if (instances.contains(instance->name)) {
			SPDLOG_WARN("Material instance with name {} already existst!", instance->name);
			return instances[instance->name];
		}

		instances[instance->name] = instance;
		return instance;
	}

	void PhongMaterial::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) {
		bool reset_required = false;
		if (config->startChild(name)) {
			reset_required |= config->addBool("shadows", &shadows);
			reset_required |= config->addBool("fresnel", &fresnel);
			reset_required |= config->addBool("dispersion", &dispersion);
			config->endChild();
		}

		if (reset_required) {
			update_flags->setFlag(TARGET_RESET);
		}
	}

	void PhongMaterial::getPushConstantValues(std::vector<int32_t> &push_constants) {
		push_constants.push_back(static_cast<int32_t>(shadows));
		push_constants.push_back(static_cast<int32_t>(fresnel));
		push_constants.push_back(static_cast<int32_t>(dispersion));
	}

	void PhongMaterial::reset() {
		instances.clear();
		Material::reset();
	}
} // namespace RtEngine
