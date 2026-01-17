//
// Created by oschdi on 5/4/25.
//

#include "Rigidbody.hpp"
#include <Node.hpp>

namespace RtEngine {
	void Rigidbody::OnUpdate() {
		auto shared_node = node.lock();
		if (!shared_node) {
			assert(false);
		}

		shared_node->transform->decomposed_transform.translation.y -= gravity * FIXED_DELTA_TIME;
	}

	void Rigidbody::definePropertySections() {
		assert(properties != nullptr);
		auto section = std::make_shared<PropertiesSection>(COMPONENT_NAME);

		section->addFloat("Gravity", &gravity);

		properties->addPropertySection(section);
	}

} // namespace RtEngine
