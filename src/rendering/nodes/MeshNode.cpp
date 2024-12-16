//
// Created by oster on 12.09.2024.
//

#include "MeshNode.hpp"

void MeshNode::draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    ctx.top_level_acceleration_structure->addInstance(meshAsset->accelerationStructure, nodeMatrix, meshAsset->objectID);

    Node::draw(topMatrix, ctx);
}
