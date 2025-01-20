//
// Created by oschdi on 12/29/24.
//

#ifndef OPTIONSUI_HPP
#define OPTIONSUI_HPP
#include <GuiWindow.hpp>
#include <bits/shared_ptr.h>
#include <glm/fwd.hpp>
#include <RaytracingOptions.hpp>

enum SceneType {
    PBR_CORNELL_BOX,
    CORNELL_BOX,
    PLANE,
    SHOWCASE,
};

struct RendererOptions {
    SceneType scene_type = PBR_CORNELL_BOX;
};

class OptionsWindow final : public GuiWindow {
public:
    OptionsWindow() : GuiWindow() {}
    explicit OptionsWindow(const std::shared_ptr<RaytracingOptions>& raytracing_options, std::shared_ptr<RendererOptions>& renderer_options)
        : GuiWindow(), raytracing_options(raytracing_options), renderer_options(renderer_options) {}

    void createFrame() override;

    std::shared_ptr<RaytracingOptions> raytracing_options;
    std::shared_ptr<RendererOptions> renderer_options;

private:
    struct ImguiOptions {
        bool shadows, fresnel, dispersion, normal_mapping;
        bool sample_light;
        int currentScene;
    };

    ImguiOptions imgui_options {
        .shadows = true,
        .fresnel = true,
        .dispersion = false,
        .normal_mapping = true,
        .sample_light = true,
    };

protected:
};



#endif //OPTIONSUI_HPP
