#ifndef BASICS_RENDERPASSBUILDER_HPP
#define BASICS_RENDERPASSBUILDER_HPP


#include <vulkan/vulkan_core.h>

namespace RtEngine {
class RenderPassBuilder {
    VkFormat colorAttachmentFormat;
    VkFormat depthFormat;

public:
    VkRenderPass createRenderPass(VkDevice device, bool addDepthAttachment = false);
    void setColorAttachmentFormat(VkFormat colorFormat);
    void setDepthFormat(VkFormat depthFormat);
};


}
#endif //BASICS_RENDERPASSBUILDER_HPP
