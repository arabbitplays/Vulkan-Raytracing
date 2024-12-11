//
// Created by oster on 05.09.2024.
//

#ifndef BASICS_DESCRIPTORALLOCATOR_HPP
#define BASICS_DESCRIPTORALLOCATOR_HPP


#include <vulkan/vulkan.h>
#include <vector>
#include <span>

class DescriptorAllocator {
public:
    struct PoolSizeRatio {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t initialSetCount, std::span<PoolSizeRatio> poolRatios);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
private:
    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool;

    const float GROW_RATIO = 1.5f;
    const uint32_t MAX_SET_COUNT = 4092;
};


#endif //BASICS_DESCRIPTORALLOCATOR_HPP
