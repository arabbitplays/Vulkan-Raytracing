//
// Created by oster on 12.09.2024.
//

#include "Node.hpp"

void Node::refreshTransform(const glm::mat4 &parentMatrix) {
    worldTransform = parentMatrix * localTransform;
    for (auto& c : children) {
        c->refreshTransform(worldTransform);
    }
}

void Node::draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
    for (auto& c : children) {
        c->draw(topMatrix, ctx);
    }
}
