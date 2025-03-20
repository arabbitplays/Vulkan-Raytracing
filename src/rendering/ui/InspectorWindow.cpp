//
// Created by oschdi on 3/20/25.
//

#include "InspectorWindow.hpp"

#include <imgui.h>
#include <bits/ranges_algo.h>

InspectorWindow::InspectorWindow(std::shared_ptr<PropertiesManager> main_props_manager)
    : GuiWindow(main_props_manager) {}


void InspectorWindow::createFrame()
{
    if (!node)
        return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    bool refresh = false;
    if (show_window) {
        ImGui::Begin("Inspector", &show_window);
        ImGui::Text(node->name.c_str());
        ImGui::Separator();

        ImGui::Text("Transform");
        refresh |= ImGui::InputFloat3("Position", &node->transform->decomposed_transform.translation[0]);
        refresh |= ImGui::InputFloat3("Rotation", &node->transform->decomposed_transform.rotation[0]);
        refresh |= ImGui::InputFloat3("Scale", &node->transform->decomposed_transform.scale[0]);

        ImGui::End();
    }

    if (refresh)
    {
        root_node->refreshTransform(glm::mat4(1.0f));
        main_props_manager->curr_sample_count = 0;
    }
}

void InspectorWindow::setNode(const std::shared_ptr<Node>& node, const std::shared_ptr<Node>& root_node)
{
    this->node = node;
    this->root_node = root_node;
}

