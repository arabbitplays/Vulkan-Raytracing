//
// Created by oster on 09.09.2024.
//

#ifndef BASICS_RESSOURCEBUILDER_HPP
#define BASICS_RESSOURCEBUILDER_HPP


#include <cstring>
#include <string>
#include <vulkan/vulkan_core.h>
#include "../rendering/engine/CommandManager.hpp"

struct AllocatedBuffer {
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory;
    uint64_t deviceAddress;
    size_t size;

    void update(VkDevice device, void* data, size_t size, size_t offset = 0) {
        void* mapped_data;
        vkMapMemory(device, bufferMemory, offset, size, 0, &mapped_data);
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
    AllocatedBuffer stageMemoryToNewBuffer(void *data, size_t size, VkBufferUsageFlags usage);
    void copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, VkDeviceSize size);
    void destroyBuffer(AllocatedBuffer buffer);

    AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                               VkImageAspectFlags aspectFlags);
    AllocatedImage createImage(void *data, VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                               VkImageAspectFlags aspectFlags, VkImageLayout target_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AllocatedImage loadTextureImage(std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    void* downloadImage(AllocatedImage image);
    void writePNG(std::string path, void* data, uint32_t width, uint32_t height);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout oldLayout, VkImageLayout newLayout);
    void transitionImageLayout(VkImage image,
        VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
        VkImageLayout oldLayout, VkImageLayout newLayout);

    void destroyImage(AllocatedImage image);

private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    CommandManager commandManager;

    void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D extent);
    void copyImageToBuffer(VkImage image, VkBuffer buffer, VkExtent3D extent);


};


#endif //BASICS_RESSOURCEBUILDER_HPP
