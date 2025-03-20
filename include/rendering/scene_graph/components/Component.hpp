//
// Created by oschdi on 3/19/25.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <IRenderable.hpp>

class Node;

class Component {
public:
    Component() = default;
    Component(std::shared_ptr<Node> node) : node(node) {};

    virtual void OnRender(DrawContext &ctx) = 0;
    virtual void OnUpdate() = 0;

    std::weak_ptr<Node> node;
};



#endif //COMPONENT_HPP
