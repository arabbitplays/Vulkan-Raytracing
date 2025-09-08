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

		pipeline->addPushConstant(MAX_PUSH_CONSTANT_SIZE, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
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
		descriptorAllocator.writeBuffer(0, resource_manager->getMaterialBuffer()->handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		auto extract_views = [](const std::vector<std::shared_ptr<Texture>> &textures) {
			std::vector<VkImageView> imageViews{};
			for (auto &texture: textures) {
				imageViews.push_back(texture->image.imageView);
			}

			// TODO make this clean so that holes get filled with the default tex
			while (imageViews.size() < 16) {
				imageViews.push_back(textures[0]->image.imageView);
			}
			return imageViews;
		};

		descriptorAllocator.writeImages(1, extract_views(albedo_textures), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
		                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorAllocator.writeImages(2, extract_views(metal_rough_ao_textures), sampler,
		                                VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		descriptorAllocator.writeImages(3, extract_views(normal_textures), sampler, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
		                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkDevice device = vulkan_context->device_manager->getDevice();
		descriptorAllocator.updateSet(device, descriptor_set->getCurrentSet());
		descriptorAllocator.clearWrites();
	}

	glm::vec4 MetalRoughMaterial::getEmissionForInstance(uint32_t material_instance_id) {
		return resource_manager->getResources()[material_instance_id]->emission;
	}


	std::vector<std::shared_ptr<MetalRoughMaterial::MetalRoughResources>> MetalRoughMaterial::getResources() const {
		return resource_manager->getResources();
	}

	std::vector<std::shared_ptr<Texture>> MetalRoughMaterial::getTextures() {
		auto append = [](std::vector<std::shared_ptr<Texture>> &dest,
		                 const std::vector<std::shared_ptr<Texture>> &src) {
			for (const auto &elem: src) {
				if (elem->path.empty())
					continue;
				dest.push_back(elem);
			}
		};

		std::vector<std::shared_ptr<Texture>> result(0);

		append(result, albedo_textures);
		append(result, metal_rough_ao_textures);
		append(result, normal_textures);

		return result;
	}

	// is unique = true the method assumes that such an instance doesn't exist yet, so safe time when creating lots of
	// instances, where it is clear that they are unique (used for loading scenes for example)
	std::shared_ptr<MaterialInstance> MetalRoughMaterial::createInstance(const MetalRoughParameters& parameters, bool unique) {
		std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
		std::shared_ptr<MetalRoughResources> resources = createMaterialResources(parameters);

		if (!unique) {
			int32_t duplicate_idx = resource_manager->isDuplicate(resources);
			if (duplicate_idx != -1) {
				assert(duplicate_idx < static_cast<int32_t>(instances.size()));
				return instances[duplicate_idx];
			}
		}

		instance->material_index = resource_manager->addResources(resources);;
		instance->properties = initializeInstanceProperties(resources);

		instances.push_back(instance);

		return instance;
	}

	void MetalRoughMaterial::reset() {
		resource_manager->reset();
		instances.clear();
		albedo_textures.clear();
		metal_rough_ao_textures.clear();
		normal_textures.clear();
		Material::reset();
	}

	void MetalRoughMaterial::initProperties() {
		properties = std::make_shared<PropertiesSection>(MATERIAL_SECTION_NAME);
		properties->addBool("Normal_Mapping", &material_properties.normal_mapping);
		properties->addBool("Sample_Lights", &material_properties.sample_lights);
		properties->addBool("Sample_BSDF", &material_properties.sample_bsdf);
		properties->addBool("Russian_Roulette", &material_properties.russian_roulette);
	}

	VkDescriptorSetLayout MetalRoughMaterial::createLayout() {
		DescriptorLayoutBuilder layoutBuilder;

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16); // TODO make this dynamic depending on the scene
		layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16);
		layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16);

		return layoutBuilder.build(vulkan_context->device_manager->getDevice(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	}

	std::shared_ptr<DescriptorSet> MetalRoughMaterial::createDescriptorSet(const VkDescriptorSetLayout &layout) {
		return std::make_shared<DescriptorSet>(vulkan_context->descriptor_allocator, vulkan_context->device_manager, layout);
	}

	std::shared_ptr<MetalRoughMaterial::MetalRoughResources>
	MetalRoughMaterial::createMaterialResources(const MetalRoughParameters &parameters) {
		auto resources = std::make_shared<MetalRoughResources>();
		resources->albedo = glm::vec4(parameters.albedo, 0.0f);
		resources->properties = glm::vec4(parameters.metallic, parameters.roughness, parameters.ao, parameters.eta);
		resources->emission = glm::vec4(parameters.emission_color, parameters.emission_power);

		auto add_texture_if_needed = [&](const std::string &texture_name,
		                                 std::vector<std::shared_ptr<Texture>> &textures) -> uint32_t {
			for (uint32_t i = 0; i < textures.size(); i++) {
				if (textures[i]->name == texture_name) {
					return i;
				}
			}

			textures.push_back(texture_repository->getTexture(texture_name));
			return textures.size() - 1;
		};

		resources->tex_indices =
				glm::vec4{add_texture_if_needed(parameters.albedo_tex_name, albedo_textures),
					add_texture_if_needed(parameters.metal_rough_ao_tex_name, metal_rough_ao_textures),
					add_texture_if_needed(parameters.normal_tex_name, normal_textures), 0};

		return resources;
	}

	std::shared_ptr<PropertiesSection>
	MetalRoughMaterial::initializeInstanceProperties(const std::shared_ptr<MetalRoughResources> &resources) {
		auto section = std::make_shared<PropertiesSection>("Metal Rough Material");
		section->addVector("Albedo", &resources->albedo);
		section->addFloat("Metal", &resources->properties.x, ALL_PROPERTY_FLAGS, 0, 1);
		section->addFloat("Roughness", &resources->properties.y, ALL_PROPERTY_FLAGS, 0, 1);
		section->addFloat("Eta", &resources->properties.w);
		section->addVector("Emission Color", reinterpret_cast<glm::vec3 *>(&resources->emission));
		section->addFloat("Emission Power", &resources->emission.w);
		return section;
	}
} // namespace RtEngine
