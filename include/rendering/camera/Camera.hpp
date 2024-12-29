//
// Created by oschdi on 12/18/24.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <deps/linmath.h>
#include <glm/glm.hpp>

class Camera {
public:
    Camera() = default;
    Camera(glm::mat4 view, glm::mat4 projection) {
        inverse_view = glm::inverse(view);
        inverse_projection = glm::inverse(projection);
        position = glm::vec3(inverse_view[3]);
    }

    virtual glm::mat4 getView();
    virtual glm::mat4 getInverseView();
    glm::mat4 getProjection();
    glm::mat4 getInverseProjection();
    glm::vec3 getPosition();

    virtual void processGlfwKeyEvent(int key, int action) {};
    virtual void processGlfwMouseEvent(double xPos, double yPos) {};
    virtual void update() {};

    uint32_t image_width;
    uint32_t image_height;

protected:
    glm::mat4 view;
    glm::mat4 inverse_view;
    glm::mat4 projection;
    glm::mat4 inverse_projection;
    glm::vec3 position;
};



#endif //CAMERA_HPP
