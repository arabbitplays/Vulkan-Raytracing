//
// Created by oschdi on 12/18/24.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

class Camera {
public:
    Camera() = default;
    Camera(glm::mat4 view, glm::mat4 projection) {
        inverse_view = glm::inverse(view);
        inverse_projection = glm::inverse(projection);
        position = glm::vec3(inverse_view[3]);
    }

    glm::mat4 view;
    glm::mat4 inverse_view;
    glm::mat4 projection;
    glm::mat4 inverse_projection;
    glm::vec3 position;
};



#endif //CAMERA_HPP
