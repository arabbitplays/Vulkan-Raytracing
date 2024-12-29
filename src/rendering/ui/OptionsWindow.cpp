//
// Created by oschdi on 12/29/24.
//

#include "OptionsWindow.hpp"

#include <imgui.h>

void OptionsWindow::createFrame() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
    if (show_window) {
        ImGui::Begin("Main Window", &show_window);
        ImGui::SliderInt("Recursion depth", &options.recursion_depth, 0, 5);
        ImGui::End();
    }
}
