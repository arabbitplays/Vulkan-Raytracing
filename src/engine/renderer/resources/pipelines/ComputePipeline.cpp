#include "ComputePipeline.hpp"

namespace RtEngine {
    void ComputePipeline::build() {
		VkDevice device = context->device_manager->getDevice();

		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = shader_stage;
		pipelineInfo.layout = layout;
		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}

		deletionQueue.pushFunction([&]() {
			vkDestroyPipelineLayout(context->device_manager->getDevice(), layout, nullptr);
			vkDestroyPipeline(context->device_manager->getDevice(), handle, nullptr);
		});
	}

	void ComputePipeline::setShaderStage(VkShaderModule shaderModule) {
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";
	}

	void ComputePipeline::setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	}

	void ComputePipeline::addPushConstant(uint32_t size, VkShaderStageFlags shaderStage) {
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = shaderStage;
		pushConstantRange.offset = 0;
		pushConstantRange.size = size;

		pushConstants.push_back(pushConstantRange);
	}

	void ComputePipeline::clear() {
		pipelineLayoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		pushConstants.clear();
	}

	void ComputePipeline::destroy() { deletionQueue.flush(); }

	VkPipeline ComputePipeline::getHandle() const { return handle; }

	VkPipelineLayout ComputePipeline::getLayoutHandle() const { return layout; }

} // RtEngine