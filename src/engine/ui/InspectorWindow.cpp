#include "InspectorWindow.hpp"

#include <imgui.h>

#include "SceneAdapter.hpp"
#include "UpdateFlagValue.hpp"

namespace RtEngine {
	InspectorWindow::InspectorWindow(const std::shared_ptr<SceneManager>& scene_manager) :
		GuiWindow(), scene_manager(scene_manager) {}

	void InspectorWindow::createFrame() {
		if (!node || !scene_manager)
			return;

		std::shared_ptr<Node> root_node = scene_manager->getCurrentScene()->nodes["root"];

		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		auto updateFlags = std::make_shared<UpdateFlags>();
		if (show_window) {
			ImGui::Begin("Inspector", &show_window);
			ImGui::Text("%s", node->name.c_str());
			ImGui::Separator();

			for (auto &component: node->components) {
				std::shared_ptr<PropertiesManager> properties = component->getProperties();
				if (!properties)
					continue;
				//refresh |= properties->serialize();
			}

			ImGui::End();
		}

		if (updateFlags->hasAny()) {
			notifyUpdate(updateFlags);
		}
	}

	void InspectorWindow::setNode(const std::shared_ptr<Node> &node) { this->node = node; }

} // namespace RtEngine
