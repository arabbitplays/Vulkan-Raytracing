#ifndef INTERACTIVECAMERA_HPP
#define INTERACTIVECAMERA_HPP

#include <Camera.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace RtEngine {
	class InteractiveCamera : public Camera {
	public:
		static constexpr float MOVE_SPEED = 0.2f;
		static constexpr float ANGULAR_MOVE_SPEED = 1 / 200.0f;

		InteractiveCamera() = default;
		InteractiveCamera(uint32_t image_width, uint32_t image_height, float fov, glm::vec3 position,
						  glm::vec3 view_dir) : Camera(image_width, image_height, fov, position, view_dir) {
			velocity = glm::vec3(0.0f);
			pitch = 0.0f;
			yaw = 0.0f;
		}

		void processGlfwKeyEvent(int key, int action) override;
		void processGlfwMouseEvent(double xPos, double yPos) override;
		void update(uint32_t image_width, uint32_t image_height) override;
		void updateViewMatrices();

		glm::vec3 velocity;
		bool isActive = true;

		// Mouse state
		float lastX = 400, lastY = 300;
		bool firstMouse = true;

		// vertical rotation
		float pitch{0.0f};
		// horizontal rotation
		float yaw{0.0f};

	private:
		glm::mat4 getRotationMatrix() const;
	};

} // namespace RtEngine
#endif // INTERACTIVECAMERA_HPP
