#include <Transform.hpp>

namespace RtEngine {
Transform::Transform() : Component(nullptr) {
    localTransform = glm::mat4(1);
    worldTransform = glm::mat4(1);
}

void Transform::setLocalTransform(glm::mat4 transform_matrix) {
	localTransform = transform_matrix;
	decomposed_transform = TransformUtil::decomposeMatrix(localTransform);
}

glm::mat4 Transform::getLocalTransform() const {
	return localTransform;
}

void Transform::updateTransforms(glm::mat4 parent_matrix)
{
	localTransform = TransformUtil::recomposeMatrix(decomposed_transform);
	worldTransform = parent_matrix * localTransform;
}


glm::mat4 Transform::getWorldTransform() const {
	return worldTransform;
}

std::shared_ptr<PropertiesManager> Transform::getProperties()
{
	if (properties != nullptr)
		return properties;

	properties = std::make_shared<PropertiesManager>();

	auto section = std::make_shared<Properties>("Transform");

	section->addVector("Position", &decomposed_transform.translation);
	section->addVector("Rotation", &decomposed_transform.rotation);
	section->addVector("Scale", &decomposed_transform.scale);

	properties->addPropertySection(section);

	return properties;
}
}
