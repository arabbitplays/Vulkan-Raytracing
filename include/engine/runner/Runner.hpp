#ifndef VULKAN_RAYTRACING_RUNNER_HPP
#define VULKAN_RAYTRACING_RUNNER_HPP
#include "SceneManager.hpp"
#include "SceneReader.hpp"
#include "VulkanRenderer.hpp"

namespace RtEngine {
    class Runner {
    public:
        Runner(std::shared_ptr<EngineContext> engine_context, const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager);

        std::string getScenePath() const;
        virtual void loadScene(const std::string &scene_path);
        virtual void renderScene();

        void setUpdateFlags(uint32_t new_flags);

        bool isRunning() const;

    protected:
        virtual void drawFrame(const std::shared_ptr<DrawContext> &draw_context);

        virtual void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context);
        virtual void finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context, uint32_t swapchain_image_idx, bool present) const;

        void handle_resize() const;

        std::shared_ptr<DrawContext> createMainDrawContext() const;

        bool running = true;

        std::shared_ptr<EngineContext> engine_context;
        std::shared_ptr<VulkanRenderer> renderer;
        std::shared_ptr<GuiManager> gui_manager;

        std::shared_ptr<SceneReader> scene_reader;
        std::shared_ptr<SceneManager> scene_manager;

        uint32_t update_flags;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RUNNER_HPP