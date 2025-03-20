//
// Created by oster on 12.09.2024.
//

#include <Node.hpp>

Node::Node()
{
    transform = std::make_shared<Transform>();
}

void Node::refreshTransform(const glm::mat4 &parentMatrix) {
    transform->updateGlobalTransform(parentMatrix);
    for (auto& c : children) {
        c->refreshTransform(transform->getWorldTransform());
    }
}

void Node::draw(DrawContext &ctx) {
    for (auto& component : components)
    {
        component->OnRender(ctx);
    }

    for (auto& c : children) {
        c->draw(ctx);
    }
}

void Node::addComponent(std::shared_ptr<Component> component)
{
    components.push_back(component);
}

