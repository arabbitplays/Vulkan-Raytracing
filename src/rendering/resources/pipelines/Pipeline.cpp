#include "../../../../include/rendering/resources/pipelines/Pipeline.hpp"

namespace RtEngine {
	void Pipeline::addShaderStage(VkShaderModule shaderModule, VkShaderStageFlagBits shaderStage) {
		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderStage;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";
		shader_stages.push_back(shaderStageInfo);
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

	void Pipeline::createPipelineLayout() {
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
		if (vkCreatePipelineLayout(context->device_manager->getDevice(), &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void Pipeline::destroy() { deletionQueue.flush(); }

	VkPipeline Pipeline::getHandle() const { return handle; }

	VkPipelineLayout Pipeline::getLayoutHandle() const { return layout; }


} // namespace RtEngine
