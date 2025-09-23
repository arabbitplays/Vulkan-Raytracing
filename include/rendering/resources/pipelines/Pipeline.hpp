#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <DeletionQueue.hpp>
#include <VulkanContext.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	class Pipeline {
	public:
		explicit Pipeline(const std::shared_ptr<VulkanContext> &context) : context(context) { }
		virtual ~Pipeline() = default;

		virtual void build() = 0;
		void addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage);
		void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);
		void addPushConstant(uint32_t size, VkShaderStageFlags shaderStage);
		void destroy();

		VkPipeline getHandle() const;
		VkPipelineLayout getLayoutHandle() const;

	protected:
		void createPipelineLayout();

		std::shared_ptr<VulkanContext> context;
		DeletionQueue deletionQueue;

		VkPipeline handle = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		std::vector<VkPushConstantRange> pushConstants{};
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
	};

} // namespace RtEngine
#endif // PIPELINE_HPP
