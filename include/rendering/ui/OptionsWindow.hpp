//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>
#include <PropertiesManager.hpp>

class OptionsWindow final : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}
    explicit OptionsWindow(std::shared_ptr<PropertiesManager> props_manager)
        : GuiWindow(), props_manager(props_manager)
    {
    }

    void createFrame() override;

    std::shared_ptr<PropertiesManager> props_manager;

private:
    struct ImguiOptions {
        bool shadows, fresnel, dispersion, normal_mapping;
        bool sample_light;
        bool sample_brdf;
        int curr_scene;
    };

    ImguiOptions imgui_options {
        .shadows = true,
        .fresnel = true,
        .dispersion = false,
        .normal_mapping = true,
        .sample_light = false,
        .sample_brdf = true,
    };


protected:
};



#endif //OPTIONSUI_HPP
