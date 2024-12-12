#ifndef BASICS_PIPELINEBUILDER_HPP
#define BASICS_PIPELINEBUILDER_HPP

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include <fstream>

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizerInfo;
    VkPipelineMultisampleStateCreateInfo multisamplingInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

    PipelineBuilder() { clear(); }

    void clear();

    void buildPipeline(VkDevice& device, VkRenderPass& renderPass, VkPipeline* pipeline, VkPipelineLayout& pipelineLayout);
    void buildPipelineLayout(VkDevice& device, VkPipelineLayout* layout);
    void setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode polygonMode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultisamplingNone();
    void enableDepthTest(VkBool32 enabled, VkCompareOp compareOp);
    void disableColorBlending();
    void enableAdditiveBlending();
    void setDescriptorSetLayouts(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void setPushConstantRanges(std::vector<VkPushConstantRange>& ranges);

private:
};


#endif //BASICS_PIPELINEBUILDER_HPP
