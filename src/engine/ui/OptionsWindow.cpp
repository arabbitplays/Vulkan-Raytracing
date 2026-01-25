#include "OptionsWindow.hpp"

#include <imgui.h>

#include "ImGuiProperties.hpp"
#include "UpdateFlags.hpp"

namespace RtEngine {
	void OptionsWindow::createFrame() {
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		bool reset_image = false;
		if (show_window) {
			ImGui::Begin("Main Window", &show_window);
			std::shared_ptr<ImGuiProperties> props = std::make_shared<ImGuiProperties>();
			for (const auto& serialize_handle : serializables) {
				serialize_handle->initProperties(props);
			}

			ImGui::End();
		}

		if (reset_image) {
			notifyUpdate(MATERIAL_UPDATE);
		}
	}

	void OptionsWindow::addSerializable(const SerializableHandle &serializable) {
		serializables.push_back(serializable);
	}
} // namespace RtEngine
