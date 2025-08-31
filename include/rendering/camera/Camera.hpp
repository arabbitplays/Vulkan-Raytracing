#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace RtEngine {
	class Camera {
	public:
		virtual ~Camera() = default;

		Camera() = default;
		Camera(const uint32_t image_width, const uint32_t image_height, const float fov, const glm::vec3 position, const glm::vec3 view_dir) :
			fov(fov), position(position), view_dir(view_dir), image_width(image_width), image_height(image_height) {
			initCamera(static_cast<float>(image_width) / static_cast<float>(image_height));
		}

		glm::mat4 getView();
		glm::mat4 getInverseView();
		glm::mat4 getProjection();
		glm::mat4 getInverseProjection();
		glm::mat4 getLastViewProjection();;
		glm::vec3 getPosition();

		virtual void processGlfwKeyEvent([[maybe_unused]] int key, [[maybe_unused]] int action) {}
		virtual void processGlfwMouseEvent([[maybe_unused]] double xPos, [[maybe_unused]] double yPos) {}

		virtual void update(const uint32_t image_width, const uint32_t image_height) {
			if (image_width != this->image_width || image_height != this->image_height) {
				initCamera(static_cast<float>(image_width) / static_cast<float>(image_height));
				this->image_width = image_width;
				this->image_height = image_height;
			}
		};

		float fov;
		glm::vec3 position, view_dir;

	protected:
		void initCamera(float aspect) {
			projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
			projection[1][1] *= -1; // flip y-axis because glm is for openGL
			view = glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));

			inverse_view = glm::inverse(view);
			inverse_projection = glm::inverse(projection);
			last_view_projection = projection * view;
			position = glm::vec3(inverse_view[3]);
		}

		glm::mat4 view;
		glm::mat4 inverse_view;
		glm::mat4 projection;
		glm::mat4 inverse_projection;
		glm::mat4 last_view_projection;

		uint32_t image_width;
		uint32_t image_height;
	};

} // namespace RtEngine
#endif // CAMERA_HPP
