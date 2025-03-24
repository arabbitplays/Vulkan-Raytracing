//
// Created by oschdi on 3/19/25.
//

#ifndef COMPONENT_HPP
#define COMPONENT_HPP
#include <IRenderable.hpp>
#include <PropertiesManager.hpp>

class Node;

class Component {
public:
    Component() = default;
    Component(std::shared_ptr<Node> node) : node(node) {};
    virtual ~Component() = default;

    virtual void OnStart() = 0;
    virtual void OnRender(DrawContext &ctx) = 0;
    virtual void OnUpdate() = 0;

    virtual std::shared_ptr<PropertiesManager> getProperties() = 0;

    std::weak_ptr<Node> node;

protected:
    std::shared_ptr<PropertiesManager> properties;
};



#endif //COMPONENT_HPP
