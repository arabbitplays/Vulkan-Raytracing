#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <DeletionQueue.hpp>
#include <VulkanContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	class RaytracingPipeline {
	public:
		RaytracingPipeline() { clear(); }
		RaytracingPipeline(const std::shared_ptr<VulkanContext> &context) : context(context) { clear(); }

		void build();
		void createShaderBindingTables(VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties);

		void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage,
							VkRayTracingShaderGroupTypeKHR shaderGroup);
		void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);
		void addPushConstant(uint32_t size, VkShaderStageFlags shaderStage);
		// Applied to all stages at build(); stages without a matching constant_id ignore it.
		void setSpecConstant(uint32_t constant_id, uint32_t value);
		void clear();
		void destroy();

		VkPipeline getHandle() const;
		VkPipelineLayout getLayoutHandle() const;
		uint32_t getGroupCount() const;

		AllocatedBuffer raygenShaderBindingTable;
		AllocatedBuffer missShaderBindingTable;
		AllocatedBuffer hitShaderBindingTable;

	private:
		std::shared_ptr<VulkanContext> context;
		DeletionQueue deletionQueue;

		VkPipeline handle = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo;
		std::vector<VkPushConstantRange> pushConstants{};
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
		std::vector<VkSpecializationMapEntry> spec_map_entries{};
		std::vector<uint32_t> spec_data{};
		VkSpecializationInfo spec_info{};
	};

} // namespace RtEngine
#endif // PIPELINE_HPP
