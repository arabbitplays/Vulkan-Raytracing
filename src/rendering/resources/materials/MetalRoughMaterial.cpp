#include "MetalRoughMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>
#include <metal_rough_closesthit.rchit.spv.h>
#include <metal_rough_miss.rmiss.spv.h>
#include <metal_rough_raygen.rgen.spv.h>
#include <shadow_miss.rmiss.spv.h>

namespace RtEngine {
	void MetalRoughMaterial::buildPipelines() {
		pipeline = std::make_shared<Pipeline>(vulkan_context);
		VkDevice device = vulkan_context->device_manager->getDevice();

		initLayout();

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts = vulkan_context->layout_manager->getLayouts();
		pipeline->setDescriptorSetLayouts(descriptorSetLayouts);

		pipeline->addPushConstant(sizeof(PushConstants), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
		                                                  VK_SHADER_STAGE_RAYGEN_BIT_KHR |
		                                                  VK_SHADER_STAGE_MISS_BIT_KHR);

		VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(
			device, oschd_metal_rough_raygen_rgen_spv_size(), oschd_metal_rough_raygen_rgen_spv());
		VkShaderModule missShaderModule = VulkanUtil::createShaderModule(
			device, oschd_metal_rough_miss_rmiss_spv_size(), oschd_metal_rough_miss_rmiss_spv());
		VkShaderModule shadowMissShaderModule = VulkanUtil::createShaderModule(
			device, oschd_shadow_miss_rmiss_spv_size(), oschd_shadow_miss_rmiss_spv());
		VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(
			device, oschd_metal_rough_closesthit_rchit_spv_size(), oschd_metal_rough_closesthit_rchit_spv());

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

	void MetalRoughMaterial::writeMaterial() {
		resource_manager->writeResources(descriptorAllocator, descriptor_set);
	}

	glm::vec4 MetalRoughMaterial::getEmissionForInstance(uint32_t material_instance_id) {
		return resource_manager->getResources()[material_instance_id]->emission;
	}


	std::vector<std::shared_ptr<MetalRoughMaterial::MetalRoughResources>> MetalRoughMaterial::getResources() const {
		return resource_manager->getResources();
	}

	std::vector<std::shared_ptr<Texture>> MetalRoughMaterial::getTextures() {
		return resource_manager->getAllTextures();
	}

	void * MetalRoughMaterial::getPushConstants(uint32_t *out_size) {
		*out_size = sizeof(PushConstants);
		return &push_constants;
	}

	void MetalRoughMaterial::resetSamples() {
		push_constants.curr_sample_count = 0;
	}

	void MetalRoughMaterial::setEmittingInstanceCount(const uint32_t count) {
		push_constants.emitting_instances_count = count;
	}

	uint32_t MetalRoughMaterial::getCurrSampleCount() {
		return push_constants.curr_sample_count;
	}

	void MetalRoughMaterial::progressSampleCount() {
		push_constants.curr_sample_count += push_constants.samples_per_pixel;
	}

	std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(
		const MetalRoughInstance::Parameters &parameters) {
		std::shared_ptr<MetalRoughInstance> instance = std::make_shared<MetalRoughInstance>(parameters, texture_repository);
		std::shared_ptr<MetalRoughResources> resources = mapInstanceToResources(*instance);

		instances.push_back(instance);

		return instance;
	}

	void MetalRoughMaterial::addInstanceToResources(MaterialInstance &inst) {
		inst.attachTo(*this);
	}

	void MetalRoughMaterial::addInstanceToResources(MetalRoughInstance &instance) {
		std::shared_ptr<MetalRoughResources> resources = mapInstanceToResources(instance);
		uint32_t material_idx = resource_manager->addResources(resources);
		instance.setMaterialDataIndex(material_idx);
	}

	void MetalRoughMaterial::reset() {
		resource_manager->reset();
		Material::reset();
	}

	void MetalRoughMaterial::initProperties() {
		properties = std::make_shared<PropertiesSection>(MATERIAL_SECTION_NAME);
		properties->addInt("Recursion_Depth", &push_constants.recursion_depth, ALL_PROPERTY_FLAGS, 1, 5);
		properties->addBool("Normal_Mapping", &push_constants.normal_mapping);
		properties->addBool("Sample_Lights", &push_constants.sample_lights);
		properties->addBool("Sample_BSDF", &push_constants.sample_bsdf);
		properties->addBool("Russian_Roulette", &push_constants.russian_roulette);
		properties->addInt("Samples_Per_Pixel", &push_constants.samples_per_pixel);
	}

	VkDescriptorSetLayout MetalRoughMaterial::createLayout() {
		DescriptorLayoutBuilder layoutBuilder;

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		defineTextureLayout(layoutBuilder, 1);
		defineTextureLayout(layoutBuilder, 2);
		defineTextureLayout(layoutBuilder, 3);

		return layoutBuilder.build(vulkan_context->device_manager->getDevice(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	}

	void MetalRoughMaterial::defineTextureLayout(DescriptorLayoutBuilder& layout_builder, uint32_t binding_idx) {
		// TODO make this dynamic depending on the scene
		layout_builder.addBinding(binding_idx, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaterialResourceManager<MetalRoughResources>::MAX_TEXTURE_COUNT);
		resource_manager->addTextureBinding(binding_idx, sampler);
	}

	std::shared_ptr<DescriptorSet> MetalRoughMaterial::createDescriptorSet(const VkDescriptorSetLayout &layout) {
		return std::make_shared<DescriptorSet>(vulkan_context->descriptor_allocator, vulkan_context->device_manager, layout);
	}

	std::shared_ptr<MetalRoughMaterial::MetalRoughResources>
	MetalRoughMaterial::mapInstanceToResources(const MetalRoughInstance &instance) const {
		auto resources = std::make_shared<MetalRoughResources>();
		resources->albedo = glm::vec4(instance.albedo, 0.0f);
		resources->properties = glm::vec4(instance.metallic, instance.roughness, instance.ao, instance.eta);
		resources->emission = glm::vec4(instance.emission_color, instance.emission_power);

		resources->tex_indices =
				glm::vec4{
					resource_manager->addTexture(1, instance.albedo_tex),
					resource_manager->addTexture(2, instance.metal_rough_ao_tex),
					resource_manager->addTexture(3, instance.normal_tex),
					0};

		return resources;
	}
} // namespace RtEngine
