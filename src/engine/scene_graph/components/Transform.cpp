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

	void Transform::definePropertySections() {
		assert(properties != nullptr);

		auto section = std::make_shared<PropertiesSection>(COMPONENT_NAME);

		section->addVector("position", &decomposed_transform.translation);
		section->addVector("rotation", &decomposed_transform.rotation);
		section->addVector("scale", &decomposed_transform.scale);

		properties->addPropertySection(section);
	}
} // namespace RtEngine
