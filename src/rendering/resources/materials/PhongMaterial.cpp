#include "PhongMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>

#include <environment_miss.rmiss.spv.h>
#include <phong_closesthit.rchit.spv.h>
#include <phong_raygen.rgen.spv.h>

#include "PhongInstance.hpp"
#include "shadow_miss.rmiss.spv.h"

namespace RtEngine {
	void PhongMaterial::buildPipelines() {
		pipeline = std::make_shared<Pipeline>(vulkan_context);
		VkDevice device = vulkan_context->device_manager->getDevice();

		initLayout();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts = vulkan_context->layout_manager->getLayouts();
		pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

		pipeline->addPushConstant(MAX_PUSH_CONSTANT_SIZE,
								  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR);

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

	VkDescriptorSetLayout PhongMaterial::createLayout() {
		DescriptorLayoutBuilder layoutBuilder;
		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		return layoutBuilder.build(vulkan_context->device_manager->getDevice(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	}

	std::shared_ptr<DescriptorSet> PhongMaterial::createDescriptorSet(const VkDescriptorSetLayout &layout) {
		return std::make_shared<DescriptorSet>(vulkan_context->descriptor_allocator, vulkan_context->device_manager, layout);
	}

	void PhongMaterial::writeMaterial() {
		resource_manager->writeResources(descriptorAllocator, descriptor_set);
	}

	std::shared_ptr<MaterialInstance> PhongMaterial::createInstance(const PhongInstance::Parameters &parameters) {
		auto instance = std::make_shared<PhongInstance>(parameters);
		std::shared_ptr<PhongResources> resources = mapInstanceToResources(*instance);

		instances.push_back(instance);
		return instance;
	}

	std::shared_ptr<PhongMaterial::PhongResources>
	PhongMaterial::mapInstanceToResources(const PhongInstance &instance) const {
		auto resources = std::make_shared<PhongResources>();
		resources->diffuse = instance.diffuse;
		resources->specular = instance.specular;
		resources->ambient = instance.ambient;

		resources->reflection = instance.reflection;
		resources->transmission = instance.transmission;
		resources->n = instance.n;
		resources->eta = glm::vec4(instance.eta, 0);

		return resources;
	}

	void PhongMaterial::addInstanceToResources(MaterialInstance &inst) {
		inst.attachTo(*this);
	}

	std::vector<std::shared_ptr<PhongMaterial::PhongResources>> PhongMaterial::getResources() const {
		return resource_manager->getResources();
	}

	void PhongMaterial::initProperties() {
		properties = std::make_shared<PropertiesSection>(MATERIAL_SECTION_NAME);
		properties->addBool("Shadows", &material_properties.shadows);
		properties->addBool("Fresnel", &material_properties.fresnel);
		properties->addBool("Dispersion", &material_properties.dispersion);
	}

	std::vector<std::shared_ptr<Texture>> PhongMaterial::getTextures() { return {}; }

	void PhongMaterial::reset() {
		resource_manager->reset();
		Material::reset();
	}
} // namespace RtEngine
