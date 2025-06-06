#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <PropertiesManager.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>

namespace RtEngine {
	class OptionsWindow final : public GuiWindow {
	public:
		OptionsWindow() : GuiWindow() {}
		explicit OptionsWindow(const std::shared_ptr<PropertiesManager>& props_manager) : GuiWindow(), main_props_manager(props_manager) {}

		void createFrame() override;

	private:
		struct ImguiOptions {
			bool shadows, fresnel, dispersion, normal_mapping;
			bool sample_light;
			bool sample_brdf;
			int curr_scene;
		};

		ImguiOptions imgui_options{
				.shadows = true,
				.fresnel = true,
				.dispersion = false,
				.normal_mapping = true,
				.sample_light = false,
				.sample_brdf = true,
		};

		std::shared_ptr<PropertiesManager> main_props_manager;
	};

} // namespace RtEngine
#endif // OPTIONSUI_HPP
