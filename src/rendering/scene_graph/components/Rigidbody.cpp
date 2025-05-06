//
// Created by oschdi on 5/4/25.
//

#include "Rigidbody.hpp"
#include <Node.hpp>

namespace RtEngine
{
void Rigidbody::OnUpdate()
{
    auto shared_node = node.lock();
    if (!shared_node) {
        assert(false);
    }

    shared_node->transform->decomposed_transform.translation.y -= gravity * FIXED_DELTA_TIME;
}

std::shared_ptr<PropertiesManager> Rigidbody::getProperties()
{
    if (properties != nullptr)
        return properties;

    properties = std::make_shared<PropertiesManager>();

    auto section = std::make_shared<PropertiesSection>("Rigidbody");

    section->addFloat("Gravity", &gravity);

    properties->addPropertySection(section);

    return properties;
}


}