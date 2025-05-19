#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <Component.hpp>
#include <TransformUtil.hpp>
#include <glm/glm.hpp>

namespace RtEngine {
class Transform : public Component {
public:
    Transform();
    static constexpr std::string COMPONENT_NAME = "Transform";

    void OnStart() override {};
    void OnRender(DrawContext& ctx) override {};
    void OnUpdate() override {};

    void definePropertySections() override;

    void setLocalTransform(glm::mat4 transform_matrix);
    glm::mat4 getLocalTransform() const;
    void updateTransforms(glm::mat4 parent_matrix);
    glm::mat4 getWorldTransform() const;

public:
    TransformUtil::DecomposedTransform decomposed_transform;

private:

    glm::mat4 localTransform{};
    glm::mat4 worldTransform{};
};



}
#endif //TRANSFORM_HPP
