#ifndef VULKAN_RAYTRACING_RUNNER_HPP
#define VULKAN_RAYTRACING_RUNNER_HPP
#include "ISerializable.hpp"
#include "SceneManager.hpp"
#include "SceneReader.hpp"
#include "RaytracingRenderer.hpp"

namespace RtEngine {
    class Runner : public ISerializable {
    public:
        Runner(std::shared_ptr<EngineContext> engine_context, const std::shared_ptr<GuiRenderer> &gui_renderer, const std::shared_ptr<SceneManager> &scene_manager);

        std::string getScenePath() const;
        virtual void loadScene(const std::string &scene_path);
        virtual void renderScene();

        void setUpdateFlags(const UpdateFlagsHandle &new_flags) const;

        bool isRunning() const;

        void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle& update_flags) override;

    protected:
        virtual void drawFrame(const std::shared_ptr<DrawContext> &draw_context);

        virtual void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context);
        virtual void finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context, uint32_t swapchain_image_idx, bool present) const;

        void handle_resize() const;

        std::shared_ptr<DrawContext> createMainDrawContext() const;

        bool running = true;
        std::string scene_name;

        std::shared_ptr<EngineContext> engine_context;
        std::shared_ptr<RaytracingRenderer> renderer;
        std::shared_ptr<GuiRenderer> gui_manager;

        std::shared_ptr<SceneReader> scene_reader;
        std::shared_ptr<SceneManager> scene_manager;

        UpdateFlagsHandle update_flags;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RUNNER_HPP