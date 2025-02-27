//
// Created by oschdi on 12/19/24.
//

#ifndef INTERACTIVECAMERA_HPP
#define INTERACTIVECAMERA_HPP

#include <Camera.hpp>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

class InteractiveCamera : public Camera {
public:
    InteractiveCamera() = default;
    InteractiveCamera(uint32_t image_width, uint32_t image_height, float fov, glm::vec3 position, glm::vec3 view_dir) : Camera(image_width, image_height, fov, position, view_dir) {
        velocity = glm::vec3(0.0f);
        pitch = 0.0f;
        yaw = 0.0f;
    }

    glm::mat4 getView() override;
    glm::mat4 getInverseView() override;

    void processGlfwKeyEvent(int key, int action) override;
    void processGlfwMouseEvent(double xPos, double yPos) override;
    void update(uint32_t image_width, uint32_t image_height) override;

    glm::vec3 velocity;
    bool isActive = true;

    // Mouse state
    float lastX = 400, lastY = 300;
    bool firstMouse = true;

    // vertical rotation
    float pitch { 0.0f };
    // horizontal rotation
    float yaw { 0.0f };

private:
    glm::mat4 getRotationMatrix();
};



#endif //INTERACTIVECAMERA_HPP
