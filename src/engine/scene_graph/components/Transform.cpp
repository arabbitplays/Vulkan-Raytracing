#include <Transform.hpp>

namespace RtEngine {
	Transform::Transform() : Component(nullptr, nullptr) {
		localTransform = glm::mat4(1);
		worldTransform = glm::mat4(1);
	}

	void Transform::setLocalTransform(glm::mat4 transform_matrix) {
		localTransform = transform_matrix;
		decomposed_transform = TransformUtil::decomposeMatrix(localTransform);
	}

	glm::mat4 Transform::getLocalTransform() const { return localTransform; }

	void Transform::updateTransforms(glm::mat4 parent_matrix) {
		localTransform = TransformUtil::recomposeMatrix(decomposed_transform);
		worldTransform = parent_matrix * localTransform;
	}

	glm::mat4 Transform::getWorldTransform() const { return worldTransform; }

	void Transform::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) {
		if (config->startChild(COMPONENT_NAME)) {
			bool refresh_needed = false;
			refresh_needed |= config->addVector("position", &decomposed_transform.translation);
			refresh_needed |= config->addVector("rotation", &decomposed_transform.rotation);
			refresh_needed |= config->addVector("scale", &decomposed_transform.scale);
			config->endChild();

			if (refresh_needed) {
				update_flags->setFlag(STATIC_GEOMETRY_UPDATE);
			}
		}
	}
} // namespace RtEngine
