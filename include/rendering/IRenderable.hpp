//
// Created by oster on 09.09.2024.
//

#include <AccelerationStructure.hpp>

#ifndef BASICS_IRENDERABLE_HPP
#define BASICS_IRENDERABLE_HPP


struct InstanceData {
    uint32_t geometry_id;
    uint32_t material_index;
};

struct EmittingInstanceData {
    glm::mat4 model_matrix;
    glm::vec4 emission;
    uint32_t instance_id;
    uint32_t primitive_count;
    uint32_t padding[2];
};

struct RenderObject {
    InstanceData instance_data;
    std::shared_ptr<AccelerationStructure> acceleration_structure;
    glm::mat4 transform;
    uint32_t primitive_count;
};

struct DrawContext {
    std::vector<RenderObject> objects;
};

class IRenderable {
    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

#endif //BASICS_IRENDERABLE_HPP

