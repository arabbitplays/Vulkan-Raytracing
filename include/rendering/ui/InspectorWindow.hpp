//
// Created by oschdi on 3/20/25.
//

#ifndef INSPECTORWINDOW_HPP
#define INSPECTORWINDOW_HPP

#include <GuiWindow.hpp>
#include <memory>
#include <Node.hpp>

class InspectorWindow : public GuiWindow {
public:
    InspectorWindow() = default;
    InspectorWindow(std::shared_ptr<PropertiesManager> main_props_manager) ;
    ~InspectorWindow() override = default;

    void createFrame() override;
    void setNode(const std::shared_ptr<Node>& node, const std::shared_ptr<Node>& root_node);

private:
    std::shared_ptr<Node> node, root_node;
};



#endif //INSPECTORWINDOW_HPP
