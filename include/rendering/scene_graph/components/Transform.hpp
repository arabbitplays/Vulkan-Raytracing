//
// Created by oschdi on 3/19/25.
//

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <Component.hpp>
#include <glm/glm.hpp>

class Transform : public Component {
public:
    Transform();

    void OnStart() override {};
    void OnRender(DrawContext& ctx) override {};
    void OnUpdate() override {};

    void setLocalTransform(glm::mat4 transform_matrix);
    glm::mat4 getLocalTransform() const;
    void updateGlobalTransform(glm::mat4 parent_matrix);
    glm::mat4 getWorldTransform() const;

private:

    glm::mat4 localTransform{};
    glm::mat4 worldTransform{};
};



#endif //TRANSFORM_HPP
