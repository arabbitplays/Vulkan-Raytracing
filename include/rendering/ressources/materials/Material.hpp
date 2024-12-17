//
// Created by oschdi on 12/17/24.
//

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <DeletionQueue.hpp>
#include <DescriptorAllocator.hpp>
#include <vulkan/vulkan_core.h>
#include <RessourceBuilder.hpp>
#include <bits/shared_ptr.h>
#include <glm/vec4.hpp>
#include <Pipeline.hpp>

class Pipeline;

struct MaterialInstance {
    std::shared_ptr<void> material_data;
    size_t data_size;
};

class Material {
  public:
    Material() = default;
    Material(VkDevice& device) : device(device) {};

    virtual ~Material() {};

    VkDevice device;
    DescriptorAllocator descriptorAllocator;
    DeletionQueue deletionQueue;

    std::shared_ptr<Pipeline> pipeline;
    VkDescriptorSetLayout materialLayout;

    virtual void buildPipelines() = 0;
    void clearRessources();
};



#endif //MATERIAL_HPP
