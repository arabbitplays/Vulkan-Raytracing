//
// Created by oschdi on 3/19/25.
//

#include <Transform.hpp>

Transform::Transform() : Component(nullptr) {
    localTransform = glm::mat4(1);
    worldTransform = glm::mat4(1);
}

void Transform::setLocalTransform(glm::mat4 transform_matrix) {
  localTransform = transform_matrix;
}

glm::mat4 Transform::getLocalTransform() const {
	return localTransform;
}

void Transform::updateGlobalTransform(glm::mat4 parent_matrix)
{
	worldTransform = parent_matrix * localTransform;
}


glm::mat4 Transform::getWorldTransform() const {
	return worldTransform;
}