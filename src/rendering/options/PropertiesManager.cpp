//
// Created by oschdi on 2/28/25.
//

#include "PropertiesManager.hpp"

#include <Material.hpp>

void PropertiesManager::addProperties(const std::shared_ptr<Properties>& section)
{
    properties[section->section_name] = section;
    updatePushConstants();
}

void* PropertiesManager::getPushConstants(uint32_t* size)
{
    updatePushConstants();
    *size = sizeof(uint32_t) * push_constants.size();
    assert(*size <= MAX_PUSH_CONSTANT_SIZE);
    return push_constants.data();
}

void PropertiesManager::updatePushConstants()
{
    push_constants.clear();

    push_constants.push_back(5);
    assert(properties.contains(MATERIAL_SECTION_NAME));
    std::shared_ptr<Properties> material_props = properties[MATERIAL_SECTION_NAME];
    for (auto& bool_option : material_props->bool_options)
    {
        push_constants.push_back(*bool_option->var);
    }

    push_constants.push_back(curr_sample_count);
    push_constants.push_back(emitting_instances_count);
}


