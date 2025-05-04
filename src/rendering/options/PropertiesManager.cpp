#include "PropertiesManager.hpp"

#include <BaseOptions.hpp>
#include <Material.hpp>
#include <ReferenceRenderer.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
PropertiesManager::PropertiesManager(const std::string& config_file_path)
{
    config = std::make_shared<ConfigLoader>(config_file_path);
}

void PropertiesManager::addPropertySection(const std::shared_ptr<Properties>& section)
{
    if (section == nullptr)
        return;
    properties[section->section_name] = section;
    if (config)
        initSectionWithConfig(section);
}

void PropertiesManager::initSectionWithConfig(const std::shared_ptr<Properties>& section)
{
    for (auto& bool_prop : section->bool_properties)
    {
        std::optional<bool> config_value = config->getConfigValue<bool>(section->section_name, bool_prop->name);
        if (config_value.has_value())
        {
            *bool_prop->var = config_value.value() ? 1 : 0;
            bool_prop->imgui_option = config_value.value();
        }
    }

    for (auto& int_prop : section->int_properties)
    {
        std::optional<int32_t> config_value = config->getConfigValue<int32_t>(section->section_name, int_prop->name);
        if (config_value.has_value())
        {
            *int_prop->var = config_value.value();
        }
    }

    for (auto& string_prop : section->string_properties)
    {
        std::optional<std::string> config_value = config->getConfigValue<std::string>(section->section_name, string_prop->name);
        if (config_value.has_value())
        {
            *string_prop->var = config_value.value();
        }
    }

    for (auto& selection_prop : section->selection_properties)
    {
        std::optional<std::string> config_value = config->getConfigValue<std::string>(section->section_name, selection_prop->name);
        if (config_value.has_value())
        {
            bool valid_value = false;
            for (int32_t i = 0; i < selection_prop->selection_options.size(); i++)
            {
                if (selection_prop->selection_options[i] == config_value.value())
                {
                    *selection_prop->var = config_value.value();
                    selection_prop->option_idx = i;
                    valid_value = true;
                    break;
                }
            }

            if (!valid_value)
                spdlog::warn("{} not a valid option for property {} in section", config_value.value(), selection_prop->name, section->section_name);
        }
    }
}


void* PropertiesManager::getPushConstants(uint32_t* size)
{
    updatePushConstants();
    *size = sizeof(uint32_t) * push_constants.size();
    assert(*size <= MAX_PUSH_CONSTANT_SIZE);
    return push_constants.data();
}

int32_t getRecursionDepth(std::vector<std::shared_ptr<IntProperty>>& int_options)
{
    for (auto& int_option : int_options)
    {
        if (int_option->name == RECURSION_DEPTH_OPTION_NAME)
        {
            return *int_option->var;
        }
    }

    return 0;
}

void PropertiesManager::updatePushConstants()
{
    push_constants.clear();
    assert(properties.contains(MATERIAL_SECTION_NAME) && properties.contains(RENDERER_SECTION_NAME));

    push_constants.push_back(getRecursionDepth(properties[RENDERER_SECTION_NAME]->int_properties));
    std::shared_ptr<Properties> material_props = properties[MATERIAL_SECTION_NAME];
    for (auto& bool_option : material_props->bool_properties)
    {
        push_constants.push_back(*bool_option->var);
    }

    push_constants.push_back(curr_sample_count);
    push_constants.push_back(emitting_instances_count);
}

bool PropertiesManager::serialize()
{
    bool change_detected = false;

    for (auto& section : properties) {
        if (ImGui::CollapsingHeader(section.second->section_name.c_str())) {
            for (auto& bool_property : section.second->bool_properties)
            {
                if (ImGui::Checkbox(bool_property->name.c_str(), &bool_property->imgui_option)) {
                    *bool_property->var = bool_property->imgui_option ? 1 : 0;
                    change_detected = true;
                }
            }

            for (auto& int_property : section.second->int_properties)
            {
                if (int_property->max > int_property->min) {
                    change_detected |= ImGui::SliderInt(int_property->name.c_str(), int_property->var, int_property->min, int_property->max);
                } else
                {
                    change_detected |=  ImGui::InputInt(int_property->name.c_str(), int_property->var);
                }
            }

            for (auto& float_property : section.second->float_properties)
            {
                if (float_property->max > float_property->min) {
                    change_detected |= ImGui::SliderFloat(float_property->name.c_str(), float_property->var, float_property->min, float_property->max);
                } else
                {
                    change_detected |=  ImGui::InputFloat(float_property->name.c_str(), float_property->var);
                }
            }

            for (auto& vector_property : section.second->vector_properties)
            {
                change_detected |= ImGui::InputFloat3(vector_property->name.c_str(), &(*vector_property->var)[0]);
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
                    change_detected = true;
                }
            }
        }
    }

    return change_detected;
}

}

