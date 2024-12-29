//
// Created by oschdi on 12/29/24.
//

#ifndef GUIWINDOW_HPP
#define GUIWINDOW_HPP



class GuiWindow {
public:
    GuiWindow() = default;

    virtual ~GuiWindow() = default;

    virtual void createFrame() = 0;

protected:
    bool show_window = true;
};



#endif //GUIWINDOW_HPP
