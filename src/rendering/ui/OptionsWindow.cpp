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

        for (auto& section : main_props_manager->properties)
        {
            if (ImGui::CollapsingHeader(section.second->section_name.c_str())) {
                for (auto& bool_property : section.second->bool_properties)
                {
                    if (ImGui::Checkbox(bool_property->name.c_str(), &bool_property->imgui_option)) {
                        *bool_property->var = bool_property->imgui_option ? 1 : 0;
                        reset_image = true;
                    }
                }

                for (auto& int_property : section.second->int_properties)
                {
                    if (int_property->max > int_property->min) {
                        if (ImGui::SliderInt(int_property->name.c_str(), int_property->var, int_property->min, int_property->max))
                        {
                            reset_image = true;
                        }
                    } else
                    {
                        if (ImGui::InputInt(int_property->name.c_str(), int_property->var))
                        {
                            reset_image = true;
                        }
                    }
                }

                for (auto& selection_property : section.second->selection_properties)
                {
                    std::vector<const char*> cStrArray;
                    for (auto& str : selection_property->selection_options) {
                        cStrArray.push_back(const_cast<char*>(str.c_str())); // Convert std::string to char*
                    }
                    const char** items = cStrArray.data();

                    if (ImGui::Combo(selection_property->name.c_str(), &selection_property->option_idx,
                            items, static_cast<int>(selection_property->selection_options.size()))) {
                        *selection_property->var= selection_property->selection_options.at(selection_property->option_idx);
                        reset_image = true;
                    }
                }
            }
        }

        ImGui::End();
    }

    if (reset_image) {
        main_props_manager->curr_sample_count = 0;
    }
}
