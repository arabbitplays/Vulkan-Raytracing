//
// Created by oster on 12.09.2024.
//

#include "MeshNode.hpp"

void MeshNode::draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (auto& surface : meshAsset->surfaces) {
        RenderObject renderObject;
        renderObject.indexCount = surface.count;
        renderObject.firstIndex = surface.startIndex;
        renderObject.vertexBuffer = meshAsset->meshBuffers.vertexBuffer.buffer;
        renderObject.indexBuffer = meshAsset->meshBuffers.indexBuffer.buffer;
        renderObject.material = &material->instance;

        renderObject.transform = nodeMatrix;

        ctx.opaqueSurfaces.push_back(renderObject);
    }

    Node::draw(topMatrix, ctx);
}
