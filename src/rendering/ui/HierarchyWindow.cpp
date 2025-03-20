//
// Created by oschdi on 3/20/25.
//

#include "HierarchyWindow.hpp"

#include <spdlog/spdlog.h>

HierarchyWindow::HierarchyWindow(std::shared_ptr<PropertiesManager> main_props_manager) : main_props_manager(main_props_manager) {}


void HierarchyWindow::setScene(const std::shared_ptr<Scene>& scene)
{
    this->scene = scene;
}


void HierarchyWindow::createFrame() {
    if (!scene)
        return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    bool reset_image = false;
    if (show_window) {
        ImGui::Begin("Hierarchy", &show_window);


        displayNode(scene->nodes["root"], nullptr, 0);

        ImGui::End();
    }

    for (auto& node_add : nodes_to_add)
    {
        scene->nodes[node_add.parent_key]->children.push_back(scene->nodes[node_add.node_key]);
        //TODO not quite working
        glm::mat4 new_parent_transform = scene->nodes[node_add.parent_key]->transform->getWorldTransform();
        glm::mat4 prev_world_transform = scene->nodes[node_add.node_key]->transform->getWorldTransform();
        scene->nodes[node_add.node_key]->transform->setLocalTransform(glm::inverse(new_parent_transform) * prev_world_transform);
        scene->nodes[node_add.node_key]->refreshTransform(new_parent_transform);
    }

    for (auto& node_remove : nodes_to_remove)
    {
        scene->nodes[node_remove.parent_key]->children.erase(
                std::remove(
                    scene->nodes[node_remove.parent_key]->children.begin(),
                    scene->nodes[node_remove.parent_key]->children.end(),
                    scene->nodes[node_remove.node_key]),
                scene->nodes[node_remove.parent_key]->children.end());
    }

    if (!nodes_to_add.empty() || !nodes_to_remove.empty())
    {
        main_props_manager->curr_sample_count = 0;
    }

    nodes_to_add.clear();
    nodes_to_remove.clear();

    scene->nodes["root"]->refreshTransform(glm::mat4(1.0f));
}

void HierarchyWindow::displayNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, uint32_t depth)
{
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * 20);

    if (ImGui::CollapsingHeader(node->name.c_str()))
    {
        for (auto& child : node->children)
        {
            displayNode(child, node, depth + 1);
        }
    }

    if (ImGui::IsItemClicked())
    {
        last_clicked_node_key = node->name;
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        dragPayload.source_key = node->name;
        dragPayload.parent_key = parent->name;
        ImGui::SetDragDropPayload("DRAG_PAYLOAD", &dragPayload, sizeof(DragPayload));
        ImGui::EndDragDropSource();
    }

    // Drop target
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* drag_payload = ImGui::AcceptDragDropPayload("DRAG_PAYLOAD");

        if (drag_payload) {
           DragPayload dragged_keys = *static_cast<DragPayload*>(drag_payload->Data);
            std::shared_ptr<Node> dragged_node = scene->nodes[dragged_keys.source_key];
            nodes_to_add.push_back({node->name, dragged_keys.source_key});
            nodes_to_remove.push_back({dragged_keys.parent_key, dragged_keys.source_key});
        }
        ImGui::EndDragDropTarget();
    }
}