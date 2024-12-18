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

class Material {
  public:
    Material() = default;
    Material(VkDevice& device) : device(device) {
        std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
        };
        descriptorAllocator.init(device, 4, poolRatios);

        deletionQueue.pushFunction([&]() {
            descriptorAllocator.destroyPools(device);
        });
    };

    virtual ~Material() {};

    std::shared_ptr<Pipeline> pipeline;
    VkDescriptorSetLayout materialLayout;
    VkDescriptorSet materialDescriptorSet;

    virtual void buildPipelines(VkDescriptorSetLayout sceneLayout) = 0;
    virtual void writeMaterial() = 0;
    void clearRessources();

protected:
    VkDevice device;
    DescriptorAllocator descriptorAllocator;
    DeletionQueue deletionQueue;
};

struct MaterialInstance {
    uint32_t material_index;
};

#endif //MATERIAL_HPP
