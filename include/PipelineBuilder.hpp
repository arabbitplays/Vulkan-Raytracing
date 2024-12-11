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

    void buildPipeline(VkDevice& device, VkRenderPass& renderPass, VkPipeline* pipeline, VkPipelineLayout* pipelineLayout);
    void setShaders(VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode polygonMode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultisamplingNone();
    void setDepthStencil();
    void disableColorBlending();
    void setDescriptorSetLayout(VkDescriptorSetLayout& descriptorSetLayout);

private:
};


#endif //BASICS_PIPELINEBUILDER_HPP
