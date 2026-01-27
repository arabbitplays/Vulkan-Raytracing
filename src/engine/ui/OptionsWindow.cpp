#include "OptionsWindow.hpp"

#include <imgui.h>

#include "ImGuiProperties.hpp"
#include "UpdateFlagValue.hpp"

namespace RtEngine {
	void OptionsWindow::createFrame() {
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		auto update_flags = std::make_shared<UpdateFlags>();
		if (show_window) {
			ImGui::Begin("Main Window", &show_window);
			std::shared_ptr<ImGuiProperties> props = std::make_shared<ImGuiProperties>();
			for (const auto& serialize_handle : serializables) {
				serialize_handle->initProperties(props, update_flags);
			}

			ImGui::End();
		}

		if (update_flags->hasAny()) {
			notifyUpdate(update_flags);
		}
	}

	void OptionsWindow::addSerializable(const SerializableHandle &serializable) {
		serializables.push_back(serializable);
	}
} // namespace RtEngine
