//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>

struct RaytracingOptions {
    int32_t recursion_depth;
};

class OptionsWindow final : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}
    explicit OptionsWindow(const std::shared_ptr<RaytracingOptions>& options) : GuiWindow(), options(options) {}

    void createFrame() override;

    std::shared_ptr<RaytracingOptions> options;

protected:
};



#endif //OPTIONSUI_HPP
