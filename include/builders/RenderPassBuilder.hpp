//
// Created by oster on 09.09.2024.
//

#ifndef BASICS_RENDERPASSBUILDER_HPP
#define BASICS_RENDERPASSBUILDER_HPP


#include <vulkan/vulkan_core.h>

class RenderPassBuilder {
    VkFormat colorAttachmentFormat;
    VkFormat depthFormat;

public:
    VkRenderPass createRenderPass(VkDevice device);
    void setColorAttachmentFormat(VkFormat colorFormat);
    void setDepthFormat(VkFormat depthFormat);
};


#endif //BASICS_RENDERPASSBUILDER_HPP
