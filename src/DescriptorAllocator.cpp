//
// Created by oster on 05.09.2024.
//

#include <stdexcept>
#include "DescriptorAllocator.hpp"

VkDescriptorPool DescriptorAllocator::getPool(VkDevice device) {
    VkDescriptorPool newPool;
    if (readyPools.size() != 0) {
        newPool = readyPools.back();
        readyPools.pop_back();
    } else {
        newPool = createPool(device, setsPerPool, ratios);

        setsPerPool *= GROW_RATIO;
        if (setsPerPool > MAX_SET_COUNT) {
            setsPerPool = MAX_SET_COUNT;
        }
    }

    return newPool;
}

VkDescriptorPool DescriptorAllocator::createPool(VkDevice device, uint32_t setCount,
                                                 std::span<DescriptorAllocator::PoolSizeRatio> poolRatios) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        VkDescriptorPoolSize poolSize {
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * setCount)
        };
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = setCount;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    return descriptorPool;
}

void DescriptorAllocator::init(VkDevice device, uint32_t initialSetCount, std::span<PoolSizeRatio> poolRatios) {
    ratios.clear();

    for (auto r : poolRatios) {
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = createPool(device, initialSetCount, poolRatios);
    setsPerPool = initialSetCount * GROW_RATIO;

    readyPools.push_back(newPool);
}

void DescriptorAllocator::clearPools(VkDevice device) {
    for (auto p : readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }

    for (auto p : fullPools) {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorAllocator::destroyPools(VkDevice device) {
    for (auto p : readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();

    for (auto p : fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout, void *pNext) {
    VkDescriptorPool poolToUse = getPool(device);

    VkDescriptorSetLayout layouts[] = {layout};
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = pNext;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
        fullPools.push_back(poolToUse);

        poolToUse = getPool(device);
        allocInfo.descriptorPool = poolToUse;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
    }

    readyPools.push_back(poolToUse);
    return descriptorSet;
}
