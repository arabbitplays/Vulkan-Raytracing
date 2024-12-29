//
// Created by oster on 09.09.2024.
//

#ifndef BASICS_DESCRIPTORLAYOUTBUILDER_HPP
#define BASICS_DESCRIPTORLAYOUTBUILDER_HPP


#include <vulkan/vulkan_core.h>
#include <vector>

class DescriptorLayoutBuilder {
public:
    void addBinding(uint32_t binding, VkDescriptorType type, uint32_t descriptor_count = 1);
    VkDescriptorSetLayout build(VkDevice device, uint32_t stageFlags);
    void clear();

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};


#endif //BASICS_DESCRIPTORLAYOUTBUILDER_HPP
