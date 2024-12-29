//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>

struct RaytracingOptions {
    int32_t recursion_depth = 3;
    int32_t shadows = 1;
    int32_t fresnel = 1;
    int32_t dispersion = 0;
};

class OptionsWindow final : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}
    explicit OptionsWindow(const std::shared_ptr<RaytracingOptions>& options) : GuiWindow(), options(options) {}

    void createFrame() override;

    std::shared_ptr<RaytracingOptions> options;

private:
    struct ImguiOptions {
        bool shadows, fresnel, dispersion;
    };

    ImguiOptions imgui_options {
        .shadows = true,
        .fresnel = true,
        .dispersion = false,
    };

protected:
};



#endif //OPTIONSUI_HPP
