//
// Created by oschdi on 5/4/25.
//

#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP
#include <Component.hpp>

namespace RtEngine
{
class Rigidbody : public Component {
public:
    Rigidbody() = default;
    Rigidbody(std::shared_ptr<Node> node) : Component(node) {};

    void OnStart() override {};
    void OnRender(DrawContext &ctx) override {};
    void OnUpdate() override;

    std::shared_ptr<PropertiesManager> getProperties() override;

private:
    float gravity = 1.0f;
};
}

#endif //RIGIDBODY_HPP
