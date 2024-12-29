//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <glm/fwd.hpp>

struct RaytracingOptions {
    int32_t recursion_depth;
};

class OptionsWindow : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}

    void createFrame() override;

    RaytracingOptions options{};

protected:
};



#endif //OPTIONSUI_HPP
