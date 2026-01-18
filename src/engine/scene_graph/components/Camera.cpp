//
// Created by oschdi on 18.01.26.
//

#include "../../../../include/engine/scene_graph/components/Camera.hpp"

namespace RtEngine {
    void Camera::OnStart() {
    }

    void Camera::OnRender(DrawContext &ctx) {
        ctx.target = render_target;
    }

    void Camera::OnUpdate() {
        VkExtent2D swapchain_extent = context->renderer->getSwapchainExtent();
        uint32_t new_image_width = swapchain_extent.width;
        uint32_t new_image_height = swapchain_extent.height;
        if (new_image_width != this->image_width || new_image_height != this->image_height) {
            updateProjection(static_cast<float>(new_image_width) / static_cast<float>(new_image_height));
            this->image_width = new_image_width;
            this->image_height = new_image_height;
        }
    }

    void Camera::definePropertySections() {
        assert(properties != nullptr);

        auto section = std::make_shared<PropertiesSection>(COMPONENT_NAME);
        section->addFloat("fov", &fov,
                                      PERSISTENT_PROPERTY_FLAG);
        properties->addPropertySection(section);
    }

    void Camera::initProperties(const YAML::Node &config_node) {
        fov = config_node[COMPONENT_NAME]["fov"].as<float>();
    }

    glm::vec3 Camera::getPosition() const { return position; }

    glm::mat4 Camera::getView() const { return view; }

    glm::mat4 Camera::getInverseView() const { return inverse_view; }

    glm::mat4 Camera::getProjection() const { return projection; }

    glm::mat4 Camera::getInverseProjection() const { return inverse_projection; }
} // RtEngine