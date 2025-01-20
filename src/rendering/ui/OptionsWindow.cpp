//
// Created by oschdi on 12/29/24.
//

#include "OptionsWindow.hpp"

#include <imgui.h>

void OptionsWindow::createFrame() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    bool reset_image = false;
    if (show_window) {
        ImGui::Begin("Main Window", &show_window);

        if (ImGui::CollapsingHeader("Renderer Options")) {
            const char* items[] = { "PBR Cornell Box", "Cornell Box", "Plane", "Showcase" };
            if (ImGui::Combo("Select an option", &imgui_options.currentScene, items, IM_ARRAYSIZE(items))) {
                switch (imgui_options.currentScene) {
                    case 0:
                        renderer_options->scene_type = SceneType::PBR_CORNELL_BOX;
                    break;
                    case 1:
                        renderer_options->scene_type = SceneType::CORNELL_BOX;
                    break;
                    case 2:
                        renderer_options->scene_type = SceneType::PLANE;
                    break;
                    case 3:
                        renderer_options->scene_type = SceneType::SHOWCASE;
                    break;
                }
            }
        }

        if (ImGui::CollapsingHeader("Raytracing Options")) {
            ImGui::SliderInt("Recursion depth", &raytracing_options->recursion_depth, 0, 5);
            if (ImGui::Checkbox("Shadows", &imgui_options.shadows)) {
                raytracing_options->shadows = imgui_options.shadows ? 1 : 0;
                reset_image = true;
            }
            if (ImGui::Checkbox("Fresnel", &imgui_options.fresnel)) {
                raytracing_options->fresnel = imgui_options.fresnel ? 1 : 0;
                reset_image = true;
            }
            if (ImGui::Checkbox("Dispersion", &imgui_options.dispersion)) {
                raytracing_options->dispersion = imgui_options.dispersion ? 1 : 0;
                reset_image = true;
            }
            if (ImGui::Checkbox("Normal Mapping", &imgui_options.normal_mapping)) {
                raytracing_options->normal_mapping = imgui_options.normal_mapping ? 1 : 0;
                reset_image = true;
            }
            if (ImGui::Checkbox("Sample lights", &imgui_options.sample_light)) {
                raytracing_options->sample_light = imgui_options.sample_light ? 1 : 0;
                reset_image = true;
            }
        }
        ImGui::End();
    }

    if (reset_image) {
        raytracing_options->curr_sample_count = 0;
    }
}
