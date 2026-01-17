#include "MetalRoughMaterial.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <OptionsWindow.hpp>
#include <VulkanUtil.hpp>
#include <metal_rough_closesthit.rchit.spv.h>
#include <metal_rough_miss.rmiss.spv.h>
#include <metal_rough_raygen.rgen.spv.h>
#include <shadow_miss.rmiss.spv.h>

#include "MetalRoughInstance.hpp"

namespace RtEngine {
	void MetalRoughMaterial::buildPipelines(VkDescriptorSetLayout sceneLayout) {
		DescriptorLayoutBuilder layoutBuilder;
		pipeline = std::make_shared<Pipeline>(vulkan_context);
		VkDevice device = vulkan_context->device_manager->getDevice();

		layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16); // TODO make this dynamic depending on the scene
		layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16);
		layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16);

		materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		mainDeletionQueue.pushFunction([&]() {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), materialLayout, nullptr);
		});

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{sceneLayout, materialLayout};
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
		if (material_buffer.handle != VK_NULL_HANDLE) {
			vulkan_context->resource_builder->destroyBuffer(material_buffer);
		}

		material_buffer = createMaterialBuffer();

		materialDescriptorSet =
				descriptorAllocator.allocate(vulkan_context->device_manager->getDevice(), materialLayout);

		descriptorAllocator.writeBuffer(0, material_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

		// TODO not needed if they are already buld corretly
		if (albedo_textures.empty()) {
			albedo_textures.push_back(runtime_context->texture_repository->getTexture("", TextureType::PARAMETER));
			metal_rough_ao_textures.push_back(runtime_context->texture_repository->getTexture("", TextureType::PARAMETER));
			normal_textures.push_back(runtime_context->texture_repository->getTexture("", TextureType::NORMAL));
		}

		auto extract_views = [](std::vector<std::shared_ptr<Texture>> textures) {
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
		descriptorAllocator.updateSet(device, materialDescriptorSet);
		descriptorAllocator.clearWrites();
	}

	std::shared_ptr<MaterialInstance> MetalRoughMaterial::loadInstance(const YAML::Node& yaml_node) {
		std::shared_ptr<MaterialInstance> instance = std::make_shared<MetalRoughInstance>();
		instance->loadResources(yaml_node);
		instances.push_back(instance);
		return instance;
	}

	void MetalRoughMaterial::initProperties() {
		properties = std::make_shared<PropertiesSection>(MATERIAL_SECTION_NAME);
		properties->addBool("Normal_Mapping", &material_properties.normal_mapping);
		properties->addBool("Sample_Lights", &material_properties.sample_lights);
		properties->addBool("Sample_BSDF", &material_properties.sample_bsdf);
		properties->addBool("Russian_Roulette", &material_properties.russian_roulette);
	}

	AllocatedBuffer MetalRoughMaterial::createMaterialBuffer() {
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

	void MetalRoughMaterial::reset() {
		instances.clear();
		albedo_textures.clear();
		metal_rough_ao_textures.clear();
		normal_textures.clear();
		Material::reset();
	}
} // namespace RtEngine
