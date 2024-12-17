//
// Created by oster on 12.09.2024.
//

#include "MeshNode.hpp"

void MeshNode::draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    ctx.objects.push_back(RenderObject{instance_id, meshAsset->accelerationStructure, nodeMatrix});

    Node::draw(topMatrix, ctx);
}
