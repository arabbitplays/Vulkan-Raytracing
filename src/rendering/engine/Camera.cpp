//
// Created by oschdi on 12/18/24.
//

#include "Camera.hpp"

glm::vec3 Camera::getPosition() {
    return position;
}

glm::mat4 Camera::getView() {
    return view;
}

glm::mat4 Camera::getInverseView() {
    return inverse_view;
}

glm::mat4 Camera::getProjection() {
    return projection;
}

glm::mat4 Camera::getInverseProjection() {
    return inverse_projection;
}


