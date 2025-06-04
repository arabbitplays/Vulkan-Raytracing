#ifndef BASICS_PIPELINEBUILDER_HPP
#define BASICS_PIPELINEBUILDER_HPP

#include <fstream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace RtEngine {
	class RasterizerPipelineBuilder {
	public:
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizerInfo;
		VkPipelineMultisampleStateCreateInfo multisamplingInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineLayoutCreateInfo pipelineLayoutInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

		RasterizerPipelineBuilder() { clear(); }

		void clear();

		void buildPipeline(VkDevice &device, VkRenderPass &renderPass, VkPipeline *pipeline,
						   VkPipelineLayout &pipelineLayout);
		void buildPipelineLayout(VkDevice &device, VkPipelineLayout *layout);
		void setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
		void setInputTopology(VkPrimitiveTopology topology);
		void setPolygonMode(VkPolygonMode polygonMode);
		void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void setMultisamplingNone();
		void enableDepthTest(VkBool32 enabled, VkCompareOp compareOp);
		void disableColorBlending();
		void enableAdditiveBlending();
		void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts);
		void setPushConstantRanges(std::vector<VkPushConstantRange> &ranges);

	private:
	};

} // namespace RtEngine
#endif // BASICS_PIPELINEBUILDER_HPP
