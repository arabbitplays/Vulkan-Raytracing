//
// Created by oster on 05.09.2024.
//

#ifndef BASICS_DESCRIPTORALLOCATOR_HPP
#define BASICS_DESCRIPTORALLOCATOR_HPP


#include <vulkan/vulkan.h>
#include <vector>
#include <span>
#include <deque>

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

    void writeBuffer(uint32_t binding, VkBuffer buffer, uint32_t size, uint32_t offset, VkDescriptorType type);
    void writeImage(uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void updateSet(VkDevice& device, VkDescriptorSet& set);
    void clearWrites();

private:
    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool;

    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    const uint32_t GROW_RATIO = 2;
    const uint32_t MAX_SET_COUNT = 4092;
};


#endif //BASICS_DESCRIPTORALLOCATOR_HPP
