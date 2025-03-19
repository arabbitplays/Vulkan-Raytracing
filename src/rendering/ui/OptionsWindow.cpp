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

        /*if (ImGui::CollapsingHeader("Raytracing Options")) {
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
            if (ImGui::Checkbox("Sample bsdf", &imgui_options.sample_brdf)) {
                raytracing_options->sample_brdf = imgui_options.sample_brdf ? 1 : 0;
                reset_image = true;
            }
        }*/

        for (auto& section : props_manager->properties)
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
        props_manager->curr_sample_count = 0;
    }
}
