#include "OptionsWindow.hpp"

#include <imgui.h>

namespace RtEngine {
	void OptionsWindow::createFrame() {
		ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

		bool reset_image = false;
		if (show_window) {
			ImGui::Begin("Main Window", &show_window);
			reset_image = main_props_manager->serialize();
			ImGui::End();
		}

		if (reset_image) {
			notifyUpdate(0);
		}
	}
} // namespace RtEngine
