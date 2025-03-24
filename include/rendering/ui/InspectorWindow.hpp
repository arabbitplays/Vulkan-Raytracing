//
// Created by oschdi on 3/20/25.
//

#ifndef INSPECTORWINDOW_HPP
#define INSPECTORWINDOW_HPP

#include <GuiWindow.hpp>
#include <memory>
#include <Node.hpp>
#include <SceneManager.hpp>
#include <VulkanContext.hpp>

class InspectorWindow : public GuiWindow {
public:
    InspectorWindow() = default;
    InspectorWindow(std::shared_ptr<PropertiesManager> main_props_manager, std::shared_ptr<SceneManager> scene_manager) ;
    ~InspectorWindow() override = default;

    void createFrame() override;
    void setNode(const std::shared_ptr<Node>& node);
private:
    std::shared_ptr<SceneManager> scene_manager;
    std::shared_ptr<Node> node;
};



#endif //INSPECTORWINDOW_HPP
