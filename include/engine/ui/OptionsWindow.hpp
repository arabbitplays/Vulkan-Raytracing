#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
 #include <bits/shared_ptr.h>
#include <glm/fwd.hpp>

#include "ISerializable.hpp"

namespace RtEngine {
	class OptionsWindow final : public GuiWindow {
	public:
		OptionsWindow() : GuiWindow() {}

		void createFrame() override;
		void addSerializable(const SerializableHandle& serializable);

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

		std::vector<SerializableHandle> serializables;
	};

} // namespace RtEngine
#endif // OPTIONSUI_HPP
