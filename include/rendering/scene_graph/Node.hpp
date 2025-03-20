//
// Created by oster on 12.09.2024.
//

#ifndef BASICS_NODE_HPP
#define BASICS_NODE_HPP


#include <IRenderable.hpp>
#include <Transform.hpp>

class Node : public IRenderable {
public:
    Node();
    ~Node() = default;

    void draw(DrawContext& ctx) override;
    void refreshTransform(const glm::mat4& parentMatrix);

    std::string name;

    std::weak_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> children;

    std::vector<std::shared_ptr<Component>> components;
    std::shared_ptr<Transform> transform;
};


#endif //BASICS_NODE_HPP
