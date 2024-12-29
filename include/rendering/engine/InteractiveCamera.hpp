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
    InteractiveCamera(glm::mat4 projection) : Camera(glm::mat4(1.0f), projection) {
        velocity = glm::vec3(0.0f);
        pitch = 0.0f;
        yaw = 0.0f;
    }

    glm::mat4 getView() override;
    glm::mat4 getInverseView() override;

    void processGlfwKeyEvent(int key, int action) override;
    void processGlfwMouseEvent(double xPos, double yPos) override;
    void update() override;

    glm::vec3 velocity;
    glm::vec3 position;
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
