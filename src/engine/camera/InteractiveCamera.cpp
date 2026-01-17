#define GLM_ENABLE_EXPERIMENTAL

#include "InteractiveCamera.hpp"

#include <GLFW/glfw3.h>
#include <glm/detail/type_quat.hpp>
#include <glm/gtx/quaternion.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	void InteractiveCamera::update(uint32_t image_width, uint32_t image_height) {
		glm::mat4 cameraRotation = getRotationMatrix();
		position += glm::vec3(cameraRotation * glm::vec4(velocity * MOVE_SPEED, 0.f));
		updateViewMatrices();
	}

	void InteractiveCamera::processGlfwKeyEvent(int key, int action) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_X) {
				isActive = !isActive;
			}
		}

		if (!isActive) {
			return;
		}

		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_W) {
				velocity.z = -1;
			}
			if (key == GLFW_KEY_S) {
				velocity.z = 1;
			}
			if (key == GLFW_KEY_A) {
				velocity.x = -1;
			}
			if (key == GLFW_KEY_D) {
				velocity.x = 1;
			}
			if (key == GLFW_KEY_SPACE) {
				velocity.y = 0.5;
			}
			if (key == GLFW_KEY_LEFT_SHIFT) {
				velocity.y = -0.5;
			}
		}

		if (action == GLFW_RELEASE) {
			if (key == GLFW_KEY_W) {
				velocity.z = 0;
			}
			if (key == GLFW_KEY_S) {
				velocity.z = 0;
			}
			if (key == GLFW_KEY_A) {
				velocity.x = 0;
			}
			if (key == GLFW_KEY_D) {
				velocity.x = 0;
			}
			if (key == GLFW_KEY_SPACE) {
				velocity.y = 0;
			}
			if (key == GLFW_KEY_LEFT_SHIFT) {
				velocity.y = 0;
			}
		}
	}

	void InteractiveCamera::processGlfwMouseEvent(double xPos, double yPos) {
		if (!isActive) {
			return;
		}

		if (firstMouse) { // Initialize first frame
			lastX = xPos;
			lastY = yPos;
			firstMouse = false;
		}

		// Calculate mouse offset
		float xOffset = xPos - lastX;
		float yOffset = lastY - yPos; // Inverted Y-axis
		lastX = xPos;
		lastY = yPos;

		// Adjust yaw and pitch like in SDL
		yaw += xOffset * ANGULAR_MOVE_SPEED;
		pitch += yOffset * ANGULAR_MOVE_SPEED;

		constexpr float max = M_PI / 2;
		pitch = std::clamp(pitch, -max, max);
	}

	void InteractiveCamera::updateViewMatrices() {
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 cameraRotation = getRotationMatrix();
		inverse_view = cameraTranslation * cameraRotation;
		view = glm::inverse(getInverseView());
	}

	glm::mat4 InteractiveCamera::getRotationMatrix() const {
		glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.0f, 0.0f, 0.0f});
		glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{0.0f, -1.0f, 0.0f});

		return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
	}
} // namespace RtEngine
