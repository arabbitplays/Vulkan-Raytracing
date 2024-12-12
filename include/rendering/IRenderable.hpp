//
// Created by oster on 09.09.2024.
//

#include <vulkan/vulkan_core.h>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP

enum class MaterialPass : uint8_t {
    MainColor,
    Transparent,
    Other
};

struct MaterialPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass materialPass;
};

struct Material {
    MaterialInstance instance;
};

struct RenderObject {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    MaterialInstance* material;

    glm::mat4 transform;
};

struct DrawContext {
    std::vector<RenderObject> opaqueSurfaces;
};

class IRenderable {
    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

#endif //BASICS_IRENDERABLE_HPP

