#include "../../../include/engine/ui/GuiManager.hpp"

namespace RtEngine {
    GuiManager::GuiManager(std::shared_ptr<SceneManager> scene_manager, const std::shared_ptr<GuiRenderer> &gui_renderer) : gui_renderer(gui_renderer) {
        options_window = std::make_shared<OptionsWindow>();
        gui_renderer->addWindow(options_window);

        inspector_window = std::make_shared<InspectorWindow>(scene_manager);
        gui_renderer->addWindow(inspector_window);

        hierarchy_window = std::make_shared<HierarchyWindow>(inspector_window, scene_manager);
        gui_renderer->addWindow(hierarchy_window);
    }

    void GuiManager::addCallbackToAll(const std::function<void(const UpdateFlagsHandle&)> &callback) const {
        options_window->addCallback(callback);
        inspector_window->addCallback(callback);
        hierarchy_window->addCallback(callback);
    }
}
