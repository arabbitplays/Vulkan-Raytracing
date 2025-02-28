//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>
#include <RaytracingOptions.hpp>
#include <RendererOptions.h>
#include <Properties.hpp>

class OptionsWindow final : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}
    explicit OptionsWindow(const std::shared_ptr<RaytracingOptions>& raytracing_options, std::shared_ptr<RendererOptions>& renderer_options)
        : GuiWindow(), raytracing_options(raytracing_options), renderer_options(renderer_options)
    {
    }

    void createFrame() override;
    void addProperties(const std::shared_ptr<Properties>& properties);

    std::shared_ptr<RaytracingOptions> raytracing_options;
    std::shared_ptr<RendererOptions> renderer_options;
    std::unordered_map<std::string, std::shared_ptr<Properties>> properties;

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
