//
// Created by oschdi on 12/29/24.
//

#include "OptionsWindow.hpp"

#include <imgui.h>

void OptionsWindow::createFrame() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
    if (show_window) {
        ImGui::Begin("Main Window", &show_window);
        ImGui::SliderInt("Recursion depth", &options->recursion_depth, 0, 5);
        if (ImGui::Checkbox("Shadows", &imgui_options.shadows)) {
            options->shadows = imgui_options.shadows ? 1 : 0;
        }
        if (ImGui::Checkbox("Fresnel", &imgui_options.fresnel)) {
            options->fresnel = imgui_options.fresnel ? 1 : 0;
        }
        if (ImGui::Checkbox("Dispersion", &imgui_options.dispersion)) {
            options->dispersion = imgui_options.dispersion ? 1 : 0;
        }
        ImGui::End();
    }
}
