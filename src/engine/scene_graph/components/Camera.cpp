//
// Created by oschdi on 18.01.26.
//

#include "../../../../include/engine/scene_graph/components/Camera.hpp"

#include <glm/gtx/quaternion.hpp>

namespace RtEngine {
    void Camera::OnStart() {

    }

    void Camera::OnRender(DrawContext &ctx) {
        ctx.target = render_target;
    }

    void Camera::OnUpdate() {
        VkExtent2D swapchain_extent = context->renderer->getSwapchainExtent();
        uint32_t new_image_width = swapchain_extent.width;
        uint32_t new_image_height = swapchain_extent.height;
        updateProjection(static_cast<float>(new_image_width) / static_cast<float>(new_image_height));
        this->image_width = new_image_width;
        this->image_height = new_image_height;

    	glm::mat4 cameraRotation = getRotationMatrix();
    	position += glm::vec3(cameraRotation * glm::vec4(velocity * MOVE_SPEED, 0.f));
    	updateViewMatrices();
    }

    void Camera::definePropertySections() {
        assert(properties != nullptr);

        auto section = std::make_shared<PropertiesSection>(COMPONENT_NAME);
        section->addFloat("fov", &fov, ALL_PROPERTY_FLAGS, 10.0f, 80.0f);
        properties->addPropertySection(section);
    	section->addBool("interactive", &is_interactive);

    	properties->addPropertySection(section);
    }

    void Camera::initProperties(const YAML::Node &config_node) {
        fov = config_node[COMPONENT_NAME]["fov"].as<float>();
    	if (config_node[COMPONENT_NAME]["interactive"] && config_node[COMPONENT_NAME]["interactive"].as<bool>())
    		is_interactive = 1;
    }

	void Camera::initCamera(float aspect) {
    	view = glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));
    	inverse_view = glm::inverse(view);
    	updateProjection(aspect);
    	position = glm::vec3(inverse_view[3]);
    }

    void Camera::processGlfwKeyEvent(int key, int action) {
		if (is_interactive == 0)
			return;

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

	void Camera::processGlfwMouseEvent(double xPos, double yPos) {
		if (!isActive || is_interactive == 0) {
			firstMouse = true;
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

	void Camera::updateProjection(float aspect) {
    	projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
    	projection[1][1] *= -1; // flip y-axis because glm is for openGL
    	inverse_projection = glm::inverse(projection);
    }

	void Camera::updateViewMatrices() {
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 cameraRotation = getRotationMatrix();
		inverse_view = cameraTranslation * cameraRotation;
		view = glm::inverse(getInverseView());
	}

	glm::mat4 Camera::getRotationMatrix() const {
		glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.0f, 0.0f, 0.0f});
		glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{0.0f, -1.0f, 0.0f});

		return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
	}

	glm::vec3 Camera::getPosition() const { return position; }

	glm::mat4 Camera::getView() const { return view; }

	glm::mat4 Camera::getInverseView() const { return inverse_view; }

	glm::mat4 Camera::getProjection() const { return projection; }

	glm::mat4 Camera::getInverseProjection() const { return inverse_projection; }
} // RtEngine