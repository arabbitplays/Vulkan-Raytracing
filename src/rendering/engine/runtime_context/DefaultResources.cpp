//
// Created by oschdi on 02.09.25.
//

#include "../../../../include/rendering/engine/runtime_context/DefaultResources.hpp"

#include "MetalRoughMaterial.hpp"
#include "PhongMaterial.hpp"

namespace RtEngine {
    DefaultResources::DefaultResources(const std::shared_ptr<VulkanContext> &vulkan_context,
		    const std::shared_ptr<TextureRepository> &texture_repository,
		    const std::shared_ptr<MaterialRepository> &material_repository) :
			vulkan_context(vulkan_context), texture_repository(texture_repository), material_repository(material_repository) {
    	createDefaultTextures();
    	createDefaultSamplers();
    	createDefaultMaterials(DeviceManager::RAYTRACING_PROPERTIES);
    }

    void DefaultResources::destroy() {
    	deletion_queue.flush();
    }

    VkSampler DefaultResources::getLinearSampler() const {
    	return defaultSamplerLinear;
    }


    void DefaultResources::createDefaultTextures() {
		uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
		whiteImage = vulkan_context->resource_builder->createImage(
				(void *) &white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
		greyImage = vulkan_context->resource_builder->createImage(
				(void *) &grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
		blackImage = vulkan_context->resource_builder->createImage(
				(void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		// checkerboard image
		const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
		std::array<uint32_t, 16 * 16> pixels{}; // for 16x16 checkerboard texture
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
			}
		}
		errorCheckerboardImage = vulkan_context->resource_builder->createImage(
				pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		deletion_queue.pushFunction([&]() {
			vulkan_context->resource_builder->destroyImage(whiteImage);
			vulkan_context->resource_builder->destroyImage(greyImage);
			vulkan_context->resource_builder->destroyImage(blackImage);
			vulkan_context->resource_builder->destroyImage(errorCheckerboardImage);
		});
	}

	void DefaultResources::createDefaultSamplers() {
		VkDevice device = vulkan_context->device_manager->getDevice();

		VkSamplerCreateInfo samplerInfo = {};
    	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vulkan_context->device_manager->getPhysicalDevice(), &properties);
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerAnisotropic) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

		deletion_queue.pushFunction([&]() {
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerLinear, nullptr);
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerNearest, nullptr);
			vkDestroySampler(vulkan_context->device_manager->getDevice(), defaultSamplerAnisotropic, nullptr);
		});
	}

	void DefaultResources::createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) {
		const auto phong_material = std::make_shared<PhongMaterial>(vulkan_context, texture_repository);
		vulkan_context->layout_manager->addLayout(1, phong_material); // todo weird to add it this way just to build the pipeline right
		phong_material->buildPipelines();
		phong_material->pipeline->createShaderBindingTables(raytracingProperties);
		material_repository->addMaterial(phong_material);

		const auto metal_rough_material =
				std::make_shared<MetalRoughMaterial>(vulkan_context, texture_repository, defaultSamplerLinear);
		vulkan_context->layout_manager->addLayout(1, metal_rough_material);
		metal_rough_material->buildPipelines();
		metal_rough_material->pipeline->createShaderBindingTables(raytracingProperties);
		material_repository->addMaterial(metal_rough_material);
	}
} // RtEngine