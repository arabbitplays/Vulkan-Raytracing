#include "Camera.hpp"

namespace RtEngine {
	glm::vec3 Camera::getPosition() const { return position; }

	glm::mat4 Camera::getInverseView() const { return inverse_view; }

	glm::mat4 Camera::getInverseProjection() const { return inverse_projection; }

	glm::mat4 Camera::getLastViewProjection() const { return last_view_projection;	}
} // namespace RtEngine
