//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_CAMERA_HPP
#define VULKAN_RAYTRACING_CAMERA_HPP
#include "Component.hpp"
#include "Node.hpp"
#include "Transform.hpp"

namespace RtEngine {
    class Camera : public Component {
    public:
        Camera() = default;
        Camera(const std::shared_ptr<EngineContext>& context, const std::shared_ptr<Node>& node) : Component(context, node) { };

        static constexpr std::string COMPONENT_NAME = "Camera";

        void OnStart() override;
        void OnRender(DrawContext &ctx) override;
        void OnUpdate() override;
        void OnDestroy() override;

        void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

        [[nodiscard]] glm::mat4 getView() const;
        [[nodiscard]] glm::mat4 getInverseView() const;
        [[nodiscard]] glm::mat4 getProjection() const;
        [[nodiscard]] glm::mat4 getInverseProjection() const;
        [[nodiscard]] glm::vec3 getPosition() const;

        std::shared_ptr<RenderTarget> getRenderTarget();

    protected:
        static constexpr float MOVE_SPEED = 0.2f;
        static constexpr float ANGULAR_MOVE_SPEED = 1 / 200.0f;

        void handleInputs();
        void updateTransform();

        void updateProjection(float aspect);
        void updateViewMatrices();

        glm::mat4 getRotationMatrix() const;

        glm::mat4 view;
        glm::mat4 inverse_view;
        glm::mat4 projection;
        glm::mat4 inverse_projection;

        uint32_t image_width = 0;
        uint32_t image_height = 0;

        std::shared_ptr<RenderTarget> render_target;

        float fov = 45.0;
        bool is_interactive = false, follow_window = false;
        std::shared_ptr<Transform> transform;

        // ----------------------- movement -----------------------

        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec3 angular_velocity = glm::vec3(0.0f);
        bool isActive = true;

        // Mouse state
        glm::vec2 last_mouse_pos = glm::vec2(0.0f);
        bool firstMouse = true;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_CAMERA_HPP