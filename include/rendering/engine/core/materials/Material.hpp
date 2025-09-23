#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <PropertiesManager.hpp>
#include <ResourceBuilder.hpp>
#include <utility>
#include <bits/shared_ptr.h>
#include <vulkan/vulkan_core.h>

#include "MaterialInstance.hpp"
#include "MaterialResourceManager.hpp"
#include "RaytracingPipeline.hpp"
#include "TextureRepository.hpp"

namespace RtEngine {
	inline void CmdTraceRaysKHR(VkDevice device, VkCommandBuffer commandBuffer,
					 const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
					 const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
					 const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
					 const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width,
					 uint32_t height, uint32_t depth) {
		auto func = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
		if (func != nullptr) {
			return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable,
						pCallableShaderBindingTable, width, height, depth);
		}
	}

	class Pipeline;

	class Material : public ILayoutProvider {
	public:
		Material() = default;
		Material(std::string name, const std::shared_ptr<VulkanContext> &vulkan_context,
				 const std::shared_ptr<TextureRepository> &texture_repository) :
			name(std::move(name)), vulkan_context(vulkan_context), texture_repository(texture_repository) {

			descriptor_allocator = std::make_shared<DescriptorAllocator>(vulkan_context->device_manager);
			std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
					{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			};
			descriptor_allocator->init(64, poolRatios);

			mainDeletionQueue.pushFunction(
					[&]() { descriptor_allocator->destroyPools(); });
		};

		virtual ~Material() = default;

		virtual void buildPipelines(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) = 0;

		virtual void addInstanceToResources(MaterialInstance &inst) = 0;

		virtual void writeMaterial() = 0;

		virtual void recordRenderToImage(VkCommandBuffer commandBuffer, const uint32_t current_frame) {
			const uint32_t handleSizeAligned =
					VulkanUtil::alignedSize(DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleSize,
											DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleAlignment);

			VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
			raygenShaderSbtEntry.deviceAddress = graphics_pipeline->raygenShaderBindingTable.deviceAddress;
			raygenShaderSbtEntry.stride = handleSizeAligned;
			raygenShaderSbtEntry.size = handleSizeAligned;

			VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
			missShaderSbtEntry.deviceAddress = graphics_pipeline->missShaderBindingTable.deviceAddress;
			missShaderSbtEntry.stride = handleSizeAligned;
			missShaderSbtEntry.size = handleSizeAligned;

			VkStridedDeviceAddressRegionKHR closestHitShaderSbtEntry{};
			closestHitShaderSbtEntry.deviceAddress = graphics_pipeline->hitShaderBindingTable.deviceAddress;
			closestHitShaderSbtEntry.stride = handleSizeAligned;
			closestHitShaderSbtEntry.size = handleSizeAligned;

			VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

			const std::vector<VkDescriptorSet> descriptor_sets = vulkan_context->layout_manager->getDescriptorSets(current_frame);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, graphics_pipeline->getHandle());
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, graphics_pipeline->getLayoutHandle(), 0,
									static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);

			uint32_t pc_size;
			void *pc_data = getPushConstants(&pc_size);
			vkCmdPushConstants(commandBuffer, graphics_pipeline->getLayoutHandle(),
							   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
									   VK_SHADER_STAGE_MISS_BIT_KHR,
							   0, pc_size, pc_data);

			CmdTraceRaysKHR(vulkan_context->device_manager->getDevice(), commandBuffer, &raygenShaderSbtEntry,
							&missShaderSbtEntry, &closestHitShaderSbtEntry, &callableShaderSbtEntry,
							vulkan_context->swapchain->extent.width, vulkan_context->swapchain->extent.height, 1);
		}

		virtual glm::vec4 getEmissionForInstance([[maybe_unused]] uint32_t material_instance_id) { return glm::vec4(0.0f); }

		virtual std::vector<std::shared_ptr<Texture>> getTextures() = 0;

		std::vector<std::shared_ptr<MaterialInstance>> getInstances() {
			return instances;
		}

		// TODO Architecture: Phong needs to implement those too (those are only here because of push constants)
		virtual void resetSamples() {}
		virtual uint32_t getCurrSampleCount() { return 0; }
		virtual void setEmittingInstanceCount([[maybe_unused]] const uint32_t count) {}

		std::shared_ptr<PropertiesSection> getProperties() {
			if (properties == nullptr) {
				initProperties();
			}
			assert(properties != nullptr);
			return properties;
		}

		void destroyResources() {
			mainDeletionQueue.flush();
			destroyLayout();
		};

		virtual void reset() {
			// intentionally empty
		}

		std::string name;

	protected:
		virtual void *getPushConstants(uint32_t *out_size) = 0;

		virtual void initProperties() = 0;
		void destroyLayout() override {
			vkDestroyDescriptorSetLayout(vulkan_context->device_manager->getDevice(), descriptor_layout, nullptr);
		}

		std::shared_ptr<RaytracingPipeline> graphics_pipeline;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<DescriptorAllocator> descriptor_allocator;
		DeletionQueue mainDeletionQueue;

		std::shared_ptr<PropertiesSection> properties;

		std::vector<std::shared_ptr<MaterialInstance>> instances;
	};

} // namespace RtEngine
#endif // MATERIAL_HPP
