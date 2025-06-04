#ifndef GUIWINDOW_HPP
#define GUIWINDOW_HPP
#include <PropertiesManager.hpp>
#include <memory>

namespace RtEngine {
	class GuiWindow {
	public:
		GuiWindow() = default;
		GuiWindow(std::shared_ptr<PropertiesManager> main_props_manager) : main_props_manager(main_props_manager){};

		virtual ~GuiWindow() = default;

		virtual void createFrame() = 0;

	protected:
		bool show_window = true;
		std::shared_ptr<PropertiesManager> main_props_manager;
	};

} // namespace RtEngine
#endif // GUIWINDOW_HPP
