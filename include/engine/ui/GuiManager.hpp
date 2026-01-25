//
// Created by oschdi on 25.01.26.
//

#ifndef VULKAN_RAYTRACING_GUIMANAGER_HPP
#define VULKAN_RAYTRACING_GUIMANAGER_HPP
#include "HierarchyWindow.hpp"
#include "InspectorWindow.hpp"
#include "SceneManager.hpp"

namespace RtEngine {
    class GuiManager {
    public:
        GuiManager(std::shared_ptr<SceneManager> scene_manager, const std::shared_ptr<GuiRenderer> &gui_renderer);

        void addCallbackToAll(const std::function<void(uint32_t)>& callback);

        std::shared_ptr<GuiRenderer> getGuiRenderer() const;

        void destroy();

        std::shared_ptr<GuiRenderer> gui_renderer;

        std::shared_ptr<OptionsWindow> options_window;
        std::shared_ptr<InspectorWindow> inspector_window;
        std::shared_ptr<HierarchyWindow> hierarchy_window;
    private:
        std::shared_ptr<SceneManager> scene_manager;
    };
}



#endif //VULKAN_RAYTRACING_GUIMANAGER_HPP