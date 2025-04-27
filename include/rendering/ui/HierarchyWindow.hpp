#ifndef HIERARCHYWINDOW_HPP
#define HIERARCHYWINDOW_HPP

#include <GuiWindow.hpp>
#include <imgui.h>
#include <InspectorWindow.hpp>
#include <memory>
#include <Scene.hpp>
#include <string>

namespace RtEngine {
struct DragPayload
{
    std::string source_key = "";
    std::string parent_key = "";
};

struct NodeAdd
{
    std::string parent_key = "";
    std::string node_key = "";
};

struct NodeRemove
{
    std::string parent_key = "";
    std::string node_key = "";
};

static DragPayload dragPayload;

class HierarchyWindow final : public GuiWindow {
public:
    HierarchyWindow() = default;
    HierarchyWindow(std::shared_ptr<PropertiesManager> main_props_manager, std::shared_ptr<InspectorWindow> inspector_window, std::shared_ptr<SceneManager> scene_manager);
    ~HierarchyWindow() override = default;

    void createFrame() override;
    std::string last_clicked_node_key = "";

private:
    void displayNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, uint32_t depth);

    std::shared_ptr<InspectorWindow> inspector_window;
    std::shared_ptr<SceneManager> scene_manager;

    std::vector<NodeAdd> nodes_to_add{};
    std::vector<NodeRemove> nodes_to_remove{};
};



}
#endif //HIERARCHYWINDOW_HPP
