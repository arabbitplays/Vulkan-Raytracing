//
// Created by oschdi on 12/18/24.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <memory>
#include <deps/linmath.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera {
public:
    Camera() = default;
    Camera(float aspect, float fov, glm::vec3 position, glm::vec3 view_dir) : position(position), view_dir(view_dir), fov(fov) {
        glm::mat4 proj = glm::perspective(glm::radians(fov),
               aspect,
               0.1f, 512.0f);
        proj[1][1] *= -1; // flip y-axis because glm is for openGL
        glm::mat4 view =  glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));

        inverse_view = glm::inverse(view);
        inverse_projection = glm::inverse(proj);
        position = glm::vec3(inverse_view[3]);
    }

    Camera(glm::mat4 view, glm::mat4 projection)
    {
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

    float fov;
    glm::vec3 position, view_dir;

protected:
    glm::mat4 view;
    glm::mat4 inverse_view;
    glm::mat4 projection;
    glm::mat4 inverse_projection;
};



#endif //CAMERA_HPP
