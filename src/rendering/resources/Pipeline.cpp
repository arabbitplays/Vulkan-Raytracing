#include "Pipeline.hpp"

#include <VulkanUtil.hpp>
#include <stdexcept>

namespace RtEngine {
	VkResult CreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation,
										  VkPipelineCache pipelineCache, uint32_t createInfoCount,
										  const VkRayTracingPipelineCreateInfoKHR *pCreateInfos,
										  const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {

		auto func = (PFN_vkCreateRayTracingPipelinesKHR) vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
		if (func != nullptr) {
			return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator,
						pPipelines);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	VkResult GetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup,
												uint32_t groupCount, size_t dataSize, void *pData) {
		auto func = (PFN_vkGetRayTracingShaderGroupHandlesKHR) vkGetDeviceProcAddr(
				device, "vkGetRayTracingShaderGroupHandlesKHR");
		if (func != nullptr) {
			return func(device, pipeline, firstGroup, groupCount, dataSize, pData);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------

	void Pipeline::build() {
		VkDevice device = context->device_manager->getDevice();

		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
		pipelineInfo.pStages = shader_stages.data();
		pipelineInfo.groupCount = static_cast<uint32_t>(shader_groups.size());
		pipelineInfo.pGroups = shader_groups.data();
		pipelineInfo.maxPipelineRayRecursionDepth = 31;
		pipelineInfo.layout = layout;
		if (CreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create ray tracing pipeline!");
		}

		deletionQueue.pushFunction([&]() {
			vkDestroyPipelineLayout(context->device_manager->getDevice(), layout, nullptr);
			vkDestroyPipeline(context->device_manager->getDevice(), handle, nullptr);
		});
	}

	void Pipeline::createShaderBindingTables(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties) {
		VkDevice device = context->device_manager->getDevice();

		std::vector<uint32_t> rgen_indices{0};
		std::vector<uint32_t> miss_indices{1, 2};
		std::vector<uint32_t> hit_indices{3};

		const uint32_t handleSize = raytracingProperties.shaderGroupHandleSize;
		const uint32_t handleAlignment = raytracingProperties.shaderGroupHandleAlignment;
		const uint32_t handleSizeAligned = VulkanUtil::alignedSize(handleSize, handleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shader_groups.size());
		const uint32_t sbt_size = groupCount * handleSizeAligned;
		const uint32_t sbtUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		const uint32_t sbtMemoryPropertyFlags =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		raygenShaderBindingTable =
				context->resource_builder->createBuffer(rgen_indices.size() * handleSizeAligned, sbtUsageFlags, sbtMemoryPropertyFlags);
		missShaderBindingTable =
				context->resource_builder->createBuffer(miss_indices.size() * handleSizeAligned, sbtUsageFlags, sbtMemoryPropertyFlags);
		hitShaderBindingTable =
				context->resource_builder->createBuffer(hit_indices.size() * handleSizeAligned, sbtUsageFlags, sbtMemoryPropertyFlags);
		deletionQueue.pushFunction([&]() {
			context->resource_builder->destroyBuffer(raygenShaderBindingTable);
			context->resource_builder->destroyBuffer(missShaderBindingTable);
			context->resource_builder->destroyBuffer(hitShaderBindingTable);
		});

		std::vector<uint8_t> shaderHandleStorage(sbt_size);
		if (GetRayTracingShaderGroupHandlesKHR(device, handle, 0, groupCount, sbt_size, shaderHandleStorage.data()) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to get shader group handles!");
		}

		auto copyHandle = [&](AllocatedBuffer &buffer, std::vector<uint32_t> &indices, uint32_t stride) {
			size_t offset = 0;
			for (uint32_t index = 0; index < indices.size(); index++) {
				buffer.update(device, shaderHandleStorage.data() + (indices[index] * handleSizeAligned), handleSize,
							  offset);
				offset += stride * sizeof(uint8_t);
			}
		};

		copyHandle(raygenShaderBindingTable, rgen_indices, handleSizeAligned);
		copyHandle(missShaderBindingTable, miss_indices, handleSizeAligned);
		copyHandle(hitShaderBindingTable, hit_indices, handleSizeAligned);
	}

	void Pipeline::addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage,
								  VkRayTracingShaderGroupTypeKHR shaderGroup) {
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderStage;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";
		shader_stages.push_back(shaderStageInfo);

		VkRayTracingShaderGroupCreateInfoKHR shaderGroupInfo{};
		shaderGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroupInfo.type = shaderGroup;
		shaderGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

		if (shaderGroup == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR) {
			shaderGroupInfo.generalShader = static_cast<uint32_t>(shader_stages.size()) - 1;
		} else if (shaderGroup == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR) {
			shaderGroupInfo.closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 1;
		}
		shader_groups.push_back(shaderGroupInfo);
	}

	void Pipeline::setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	}

	void Pipeline::addPushConstant(uint32_t size, VkShaderStageFlags shaderStage) {
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = shaderStage;
		pushConstantRange.offset = 0;
		pushConstantRange.size = size;

		pushConstants.push_back(pushConstantRange);
	}

	void Pipeline::clear() {
		pipelineLayoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		shader_stages.clear();
		shader_groups.clear();
		pushConstants.clear();
	}

	void Pipeline::destroy() { deletionQueue.flush(); }

	VkPipeline Pipeline::getHandle() const { return handle; }

	VkPipelineLayout Pipeline::getLayoutHandle() const { return layout; }

	uint32_t Pipeline::getGroupCount() const { return shader_groups.size(); }

} // namespace RtEngine
