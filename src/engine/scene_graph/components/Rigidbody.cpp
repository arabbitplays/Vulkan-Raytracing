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

	void Rigidbody::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) {
		if (config->startChild(COMPONENT_NAME)) {
			config->addFloat("Gravity", &gravity);
		}
	}
} // namespace RtEngine
