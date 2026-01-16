#include "Camera.hpp"

namespace RtEngine {
	glm::vec3 Camera::getPosition() const { return position; }

	glm::mat4 Camera::getView() const { return view; }

	glm::mat4 Camera::getInverseView() const { return inverse_view; }

	glm::mat4 Camera::getProjection() const { return projection; }

	glm::mat4 Camera::getInverseProjection() const { return inverse_projection; }

} // namespace RtEngine
