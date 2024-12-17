//
// Created by oster on 09.09.2024.
//

#include <AccelerationStructure.hpp>
#include <Material.hpp>
#include <Pipeline.hpp>
#include <vulkan/vulkan_core.h>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP




struct RenderObject {
    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;

    MaterialInstance* material;

    glm::mat4 transform;
};

struct DrawContext {
    std::shared_ptr<AccelerationStructure> top_level_acceleration_structure = nullptr;
};

class IRenderable {
    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

#endif //BASICS_IRENDERABLE_HPP

