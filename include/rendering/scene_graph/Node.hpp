//
// Created by oster on 12.09.2024.
//

#ifndef BASICS_NODE_HPP
#define BASICS_NODE_HPP


#include <chrono>
#include <glm/glm.hpp>
#include "../IRenderable.hpp"

class Node : public IRenderable {
public:
    Node() = default;
    virtual ~Node() = default;  // Virtual destructor

    void refreshTransform(const glm::mat4& parentMatrix);
    virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx);

    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;
    
    glm::mat4 localTransform;
    glm::mat4 worldTransform;
};


#endif //BASICS_NODE_HPP
