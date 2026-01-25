#ifndef GUIWINDOW_HPP
#define GUIWINDOW_HPP
#include <deque>
#include <functional>
#include <PropertiesManager.hpp>
#include <memory>

#include "UpdateFlagValue.hpp"

namespace RtEngine {
	class GuiWindow {
	public:
		GuiWindow() = default;

		virtual ~GuiWindow() = default;

		virtual void createFrame() = 0;

		void addCallback(const std::function<void(const UpdateFlagsHandle&)>& callback)
		{
			update_callbacks.push_back(callback);
		}

	protected:
		void notifyUpdate(const UpdateFlagsHandle& flags)
		{
			for (auto it = update_callbacks.rbegin(); it != update_callbacks.rend(); it++) {
				(*it)(flags);
			}
		}

		bool show_window = true;

		std::deque<std::function<void(const UpdateFlagsHandle&)>> update_callbacks;
	};

} // namespace RtEngine
#endif // GUIWINDOW_HPP
