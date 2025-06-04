#include "HierarchyWindow.hpp"

#include <spdlog/spdlog.h>

namespace RtEngine {
	HierarchyWindow::HierarchyWindow(std::shared_ptr<PropertiesManager> main_props_manager,
									 std::shared_ptr<InspectorWindow> inspector_window,
									 std::shared_ptr<SceneManager> scene_manager) :
		GuiWindow(main_props_manager), inspector_window(inspector_window), scene_manager(scene_manager) {}

	void HierarchyWindow::createFrame() {
		if (!scene_manager || !scene_manager->scene)
			return;

		std::shared_ptr<Scene> scene = scene_manager->scene;

		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		if (show_window) {
			ImGui::Begin("Hierarchy", &show_window);
			displayNode(scene->nodes["root"], nullptr, 0);
			ImGui::End();
		}

		for (auto &node_add: nodes_to_add) {
			scene->nodes[node_add.parent_key]->children.push_back(scene->nodes[node_add.node_key]);
			// TODO not quite working
			glm::mat4 new_parent_transform = scene->nodes[node_add.parent_key]->transform->getWorldTransform();
			glm::mat4 prev_world_transform = scene->nodes[node_add.node_key]->transform->getWorldTransform();
			scene->nodes[node_add.node_key]->transform->setLocalTransform(glm::inverse(new_parent_transform) *
																		  prev_world_transform);
		}

		for (auto &node_remove: nodes_to_remove) {
			scene->nodes[node_remove.parent_key]->children.erase(
					std::remove(scene->nodes[node_remove.parent_key]->children.begin(),
								scene->nodes[node_remove.parent_key]->children.end(),
								scene->nodes[node_remove.node_key]),
					scene->nodes[node_remove.parent_key]->children.end());
		}

		std::shared_ptr<Node> root_node = scene->nodes["root"];

		if (!nodes_to_add.empty() || !nodes_to_remove.empty()) {
			main_props_manager->curr_sample_count = 0;
		}

		nodes_to_add.clear();
		nodes_to_remove.clear();

		root_node->refreshTransform(glm::mat4(1.0f));

		if (last_clicked_node_key != "")
			inspector_window->setNode(scene->nodes[last_clicked_node_key]);
		last_clicked_node_key = "";
	}

	void HierarchyWindow::displayNode(std::shared_ptr<Node> node, std::shared_ptr<Node> parent, uint32_t depth) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		// Highlight if selected
		if (last_clicked_node_key == node->name) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool open = ImGui::TreeNodeEx(node->name.c_str(), flags);

		// Register clicks directly on the TreeNode
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			last_clicked_node_key = node->name;
		}

		if (node->name != "root" && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
			dragPayload.source_key = node->name;
			dragPayload.parent_key = parent->name;
			ImGui::SetDragDropPayload("DRAG_PAYLOAD", &dragPayload, sizeof(DragPayload));
			ImGui::EndDragDropSource();
		}

		// Drop target
		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload *drag_payload = ImGui::AcceptDragDropPayload("DRAG_PAYLOAD");

			if (drag_payload) {
				DragPayload dragged_keys = *static_cast<DragPayload *>(drag_payload->Data);
				std::shared_ptr<Node> dragged_node = scene_manager->scene->nodes[dragged_keys.source_key];
				nodes_to_add.push_back({node->name, dragged_keys.source_key});
				nodes_to_remove.push_back({dragged_keys.parent_key, dragged_keys.source_key});
			}
			ImGui::EndDragDropTarget();
		}

		if (open) {
			for (auto &child: node->children) {
				displayNode(child, node, depth + 1);
			}
			ImGui::TreePop();
		}
	}
} // namespace RtEngine
