//
// Created by oschdi on 12/19/24.
//
#define GLM_ENABLE_EXPERIMENTAL

#include "InteractiveCamera.hpp"

#include <GLFW/glfw3.h>
#include <glm/detail/type_quat.hpp>
#include <glm/gtx/quaternion.hpp>

void InteractiveCamera::update() {
    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}

void InteractiveCamera::processGlfwKeyEvent(int key, int action) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_W) { velocity.z = -1; }
        if (key == GLFW_KEY_S) { velocity.z = 1; }
        if (key == GLFW_KEY_A) { velocity.x = -1; }
        if (key == GLFW_KEY_D) { velocity.x = 1; }
    }

    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_W) { velocity.z = 0; }
        if (key == GLFW_KEY_S) { velocity.z = 0; }
        if (key == GLFW_KEY_A) { velocity.x = 0; }
        if (key == GLFW_KEY_D) { velocity.x = 0; }
    }
}

void InteractiveCamera::processGlfwMouseEvent(double xPos, double yPos) {
    if (firstMouse) {  // Initialize first frame
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    // Calculate mouse offset
    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;  // Inverted Y-axis
    lastX = xPos;
    lastY = yPos;

    // Adjust yaw and pitch like in SDL
    yaw -= xOffset / 200.f;
    pitch += yOffset / 200.f;

    // Clamp pitch to avoid flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}


glm::mat4 InteractiveCamera::getView() {
    // the view matrix is the opposite of the camera transform calculated in getInverseView()
    return glm::inverse(getInverseView());
}

glm::mat4 InteractiveCamera::getInverseView() {
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 cameraRotation = getRotationMatrix();
    return cameraTranslation * cameraRotation;
}



glm::mat4 InteractiveCamera::getRotationMatrix() {
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3{1.0f, 0.0f, 0.0f});
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3{0.0f, -1.0f, 0.0f});

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

