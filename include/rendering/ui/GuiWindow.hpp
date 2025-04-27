#ifndef GUIWINDOW_HPP
#define GUIWINDOW_HPP
#include <memory>
#include <PropertiesManager.hpp>


namespace RtEngine {
class GuiWindow {
public:
    GuiWindow() = default;
    GuiWindow(std::shared_ptr<PropertiesManager> main_props_manager) : main_props_manager(main_props_manager) {};

    virtual ~GuiWindow() = default;

    virtual void createFrame() = 0;

protected:
    bool show_window = true;
    std::shared_ptr<PropertiesManager> main_props_manager;
};



}
#endif //GUIWINDOW_HPP
