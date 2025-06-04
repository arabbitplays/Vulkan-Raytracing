#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <deps/linmath.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace RtEngine {
	class Camera {
	public:
		Camera() = default;
		Camera(uint32_t image_width, uint32_t image_height, float fov, glm::vec3 position, glm::vec3 view_dir) :
			position(position), view_dir(view_dir), fov(fov), image_width(image_width), image_height(image_height) {
			initCamera(static_cast<float>(image_width) / static_cast<float>(image_height));
		}

		virtual glm::mat4 getView();
		virtual glm::mat4 getInverseView();
		glm::mat4 getProjection();
		glm::mat4 getInverseProjection();
		glm::vec3 getPosition();

		virtual void processGlfwKeyEvent(int key, int action) {};
		virtual void processGlfwMouseEvent(double xPos, double yPos) {};
		virtual void update(uint32_t image_width, uint32_t image_height) {
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
			glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
			proj[1][1] *= -1; // flip y-axis because glm is for openGL
			glm::mat4 view = glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));

			inverse_view = glm::inverse(view);
			inverse_projection = glm::inverse(proj);
			position = glm::vec3(inverse_view[3]);
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
