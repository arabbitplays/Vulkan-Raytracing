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

		void buildPipeline(const VkDevice &device, const VkRenderPass &render_pass, VkPipeline *pipeline,
						   const VkPipelineLayout &pipeline_layout);
		void buildPipelineLayout(const VkDevice &device, VkPipelineLayout *pipeline_layout) const;
		void setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
		void setInputTopology(VkPrimitiveTopology topology);
		void setPolygonMode(VkPolygonMode polygonMode);
		void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
		void setMultisamplingNone();
		void enableDepthTest(VkBool32 enabled, VkCompareOp compareOp);
		void disableColorBlending();
		void enableAdditiveBlending();
		void setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &descriptor_set_layouts);
		void setPushConstantRanges(const std::vector<VkPushConstantRange> &ranges);

	private:
	};

} // namespace RtEngine
#endif // BASICS_PIPELINEBUILDER_HPP
