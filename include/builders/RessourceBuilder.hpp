//
// Created by oster on 09.09.2024.
//

#ifndef BASICS_RESSOURCEBUILDER_HPP
#define BASICS_RESSOURCEBUILDER_HPP


#include <cstring>
#include <vulkan/vulkan_core.h>
#include "../rendering/engine/CommandManager.hpp"

struct AllocatedBuffer {
    VkBuffer handle;
    VkDeviceMemory bufferMemory;
    uint64_t deviceAddress;

    void update(VkDevice device, void* data, size_t size) {
        void* mapped_data;
        vkMapMemory(device, bufferMemory, 0, size, 0, &mapped_data);
        memcpy(mapped_data, data, size);
        vkUnmapMemory(device, bufferMemory);
    }
};

struct AllocatedImage {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkFormat imageFormat;
    VkExtent3D imageExtent;
};

class RessourceBuilder {
public:
    RessourceBuilder() = default;
    RessourceBuilder(VkPhysicalDevice physicalDevice, VkDevice device, CommandManager commandManager);
    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, VkDeviceSize size);
    void destroyBuffer(AllocatedBuffer buffer);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageLayout initialLayout,
                                VkImageUsageFlags usage, VkImageAspectFlags aspectFlags);
    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                               VkImageAspectFlags aspectFlags);
    AllocatedImage createImage(void *data, VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                               VkImageAspectFlags aspectFlags);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void destroyImage(AllocatedImage image);

private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    CommandManager commandManager;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D extent);

};


#endif //BASICS_RESSOURCEBUILDER_HPP
