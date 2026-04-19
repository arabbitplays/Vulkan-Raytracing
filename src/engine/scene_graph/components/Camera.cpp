#include "Camera.hpp"

#include <glm/gtx/quaternion.hpp>

#include "Node.hpp"

namespace RtEngine {
    void Camera::OnStart() {

		if (image_height == 0 || image_width == 0) {
			follow_window = true;

			VkExtent2D swapchain_extent = context->swapchain_manager->getSwapchainExtent();
			image_width = swapchain_extent.width;
			image_height = swapchain_extent.height;

			context->swapchain_manager->addRecreateCallback([this] (uint32_t new_width, uint32_t new_height) {
				image_width = new_width;
				image_height = new_height;
				if (render_target != nullptr) {
					render_target->recreate(VkExtent2D{static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height)});
				}
			});
		}

    	render_target = context->rendering_manager->createRenderTarget(image_width, image_height);
    	transform = node.lock()->transform;
    }

    void Camera::OnRender(DrawContext &ctx) {
        ctx.targets.push_back(render_target);
    }

    void Camera::OnUpdate() {
    	if (is_interactive) {
    		handleInputs();
    	}

    	updateTransform();
    	updateViewMatrices();
    	updateProjection(static_cast<float>(image_width) / static_cast<float>(image_height));
    }

    void Camera::OnDestroy() {
    	render_target->destroy();
    }

    void Camera::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) {
    	bool requires_reset = false;
    	if (config->startChild(COMPONENT_NAME)) {
    		requires_reset |= config->addFloat("fov", &fov, 10.0f, 80.0f);
    		requires_reset |= config->addBool("interactive", &is_interactive);
    		if (!follow_window) {
    			requires_reset |= config->addUint("width", &image_width);
    			requires_reset |= config->addUint("height", &image_height);
    		}
    		config->endChild();
    	}

    	if (requires_reset) {
    		update_flags->setFlag(TARGET_RESET);
    	}
    }

	void Camera::handleInputs() {
    	std::shared_ptr<InputManager> input_manager = context->input_manager;
    	if (input_manager->getKeyDown(Keycode::X)) {
    		isActive = !isActive;
    	}

    	if (!isActive) {
    		firstMouse = true;
    		return;
    	}

    	if (input_manager->getKeyDown(Keycode::W))
    		velocity.z = -1;
    	if (input_manager->getKeyUp(Keycode::W))
    		velocity.z = 0;

    	if (input_manager->getKeyDown(Keycode::S))
    		velocity.z = 1;
    	if (input_manager->getKeyUp(Keycode::S))
    		velocity.z = 0;

    	if (input_manager->getKeyDown(Keycode::A))
    		velocity.x = -1;
    	if (input_manager->getKeyUp(Keycode::A))
    		velocity.x = 0;

    	if (input_manager->getKeyDown(Keycode::D))
    		velocity.x = 1;
    	if (input_manager->getKeyUp(Keycode::D))
    		velocity.x = 0;

    	if (input_manager->getKeyDown(Keycode::SHIFT))
    		velocity.y = -0.5f;
    	if (input_manager->getKeyUp(Keycode::SHIFT))
    		velocity.y = 0;

    	if (input_manager->getKeyDown(Keycode::SPACE))
    		velocity.y = 0.5f;
    	if (input_manager->getKeyUp(Keycode::SPACE))
    		velocity.y = 0;


    	glm::vec2 mouse_pos = input_manager->getMousePosition();

    	if (firstMouse && glm::length(mouse_pos) > 0.001f) { // Initialize first frame
    		last_mouse_pos = mouse_pos;
    		firstMouse = false;
    	}

    	glm::vec2 offset = mouse_pos - last_mouse_pos;
    	offset.y *= -1; // Inverted Y-axis

    	last_mouse_pos = mouse_pos;

    	// Adjust yaw and pitch like in SDL
    	angular_velocity.y += offset.x * ANGULAR_MOVE_SPEED;
    	angular_velocity.x += offset.y * ANGULAR_MOVE_SPEED;
    }

	void Camera::updateTransform() {
    	transform->decomposed_transform.rotation += angular_velocity;
    	constexpr float max = M_PI / 2;
    	transform->decomposed_transform.rotation.x = std::clamp(transform->decomposed_transform.rotation.x, -max, max);

    	glm::mat4 cameraRotation = getRotationMatrix();
    	transform->decomposed_transform.translation += glm::vec3(cameraRotation * glm::vec4(velocity * MOVE_SPEED, 0.f));

    	angular_velocity = glm::vec3(0);
    }

	void Camera::updateProjection(float aspect) {
    	projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
    	projection[1][1] *= -1; // flip y-axis because glm is for openGL
    	inverse_projection = glm::inverse(projection);
    }

	void Camera::updateViewMatrices() {
		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), transform->decomposed_transform.translation);
		glm::mat4 cameraRotation = getRotationMatrix();
		inverse_view = cameraTranslation * cameraRotation;
		view = glm::inverse(getInverseView());
	}

	glm::mat4 Camera::getRotationMatrix() const {
    	float pitch = transform->decomposed_transform.rotation.x;
    	float yaw = transform->decomposed_transform.rotation.y;
		glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.0f, 0.0f, 0.0f});
		glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{0.0f, -1.0f, 0.0f});

		return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
	}

	glm::vec3 Camera::getPosition() const { return transform->decomposed_transform.translation; }

	std::shared_ptr<RenderTarget> Camera::getRenderTarget() {
    	return render_target;
	}

	glm::mat4 Camera::getView() const { return view; }

	glm::mat4 Camera::getInverseView() const { return inverse_view; }

	glm::mat4 Camera::getProjection() const { return projection; }

	glm::mat4 Camera::getInverseProjection() const { return inverse_projection; }
} // RtEngine