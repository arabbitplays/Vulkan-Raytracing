//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_CAMERA_HPP
#define VULKAN_RAYTRACING_CAMERA_HPP
#include "Component.hpp"

namespace RtEngine {
    class Camera : public Component {
    public:
        Camera() = default;
        Camera(const std::shared_ptr<EngineContext>& context, const std::shared_ptr<Node>& node) : Component(context, node) {
            VkExtent2D swapchain_extent = context->renderer->getSwapchainExtent();
            image_width = swapchain_extent.width;
            image_height = swapchain_extent.height;
            initCamera(static_cast<float>(image_width) / static_cast<float>(image_height));

            context->window->addKeyCallback([this](int key, int scancode, int action, int mods) {
                //processGlfwKeyEvent(key, action);
            });
            context->window->addMouseCallback([this](double xPos, double yPos) {
                //processGlfwMouseEvent(xPos, yPos);
            });

            render_target = context->renderer->createRenderTarget();
        };

        static constexpr std::string COMPONENT_NAME = "Camera";

        void OnStart() override;
        void OnRender(DrawContext &ctx) override;
        void OnUpdate() override;

        void definePropertySections() override;
        void initProperties(const YAML::Node &config_node) override;

        [[nodiscard]] glm::mat4 getView() const;
        [[nodiscard]] glm::mat4 getInverseView() const;
        [[nodiscard]] glm::mat4 getProjection() const;
        [[nodiscard]] glm::mat4 getInverseProjection() const;
        [[nodiscard]] glm::vec3 getPosition() const;

        float fov = 45.0;
        glm::vec3 position = glm::vec3(0, 0, 10), view_dir = glm::vec3(0, 0, -1);

    protected:
        void initCamera(float aspect) {
            view = glm::lookAt(position, position + view_dir, glm::vec3(0, 1, 0));
            inverse_view = glm::inverse(view);
            updateProjection(aspect);
            position = glm::vec3(inverse_view[3]);
        }

        void updateProjection(float aspect) {
            projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 512.0f);
            projection[1][1] *= -1; // flip y-axis because glm is for openGL
            inverse_projection = glm::inverse(projection);
        }

        glm::mat4 view;
        glm::mat4 inverse_view;
        glm::mat4 projection;
        glm::mat4 inverse_projection;

        uint32_t image_width;
        uint32_t image_height;

        std::shared_ptr<RenderTarget> render_target;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_CAMERA_HPP