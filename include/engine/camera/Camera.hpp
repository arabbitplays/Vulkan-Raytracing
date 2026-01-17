#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace RtEngine {
	class Camera {
	public:
		Camera() = default;
		Camera(float fov, glm::vec3 position, glm::vec3 view_dir) :
			position(position), view_dir(view_dir), fov(fov), image_width(1), image_height(1) {
			initCamera(static_cast<float>(image_width) / static_cast<float>(image_height));
		}

		[[nodiscard]] glm::mat4 getView() const;
		[[nodiscard]] glm::mat4 getInverseView() const;
		[[nodiscard]] glm::mat4 getProjection() const;
		[[nodiscard]] glm::mat4 getInverseProjection() const;
		[[nodiscard]] glm::vec3 getPosition() const;

		virtual void processGlfwKeyEvent(int key, int action) {};
		virtual void processGlfwMouseEvent(double xPos, double yPos) {};
		virtual void update(const uint32_t new_image_width, const uint32_t new_image_height) {
			if (new_image_width != this->image_width || new_image_height != this->image_height) {
				updateProjection(static_cast<float>(new_image_width) / static_cast<float>(new_image_height));
				this->image_width = new_image_width;
				this->image_height = new_image_height;
			}
		};

		float fov;
		glm::vec3 position, view_dir;

	protected:
		void initCamera(float aspect) {
			view = glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));
			inverse_view = glm::inverse(view);
			updateProjection(aspect);
			position = glm::vec3(inverse_view[3]);
		}

		void updateProjection(float aspect) {
			projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
			projection[1][1] *= -1; // flip y-axis because glm is for openGL
			inverse_projection = glm::inverse(projection);
		}

		glm::mat4 view;
		glm::mat4 inverse_view;
		glm::mat4 projection;
		glm::mat4 inverse_projection;

		uint32_t image_width;
		uint32_t image_height;
	};

} // namespace RtEngine
#endif // CAMERA_HPP
