//
// Created by oster on 09.09.2024.
//

#include <stdexcept>
#include "RessourceBuilder.hpp"

#include <cstring>

VkDeviceAddress GetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfoKHR* address_info) {
    auto func = (PFN_vkGetBufferDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
    if (func != nullptr) {
        return func(device, address_info);
    }
    else {
        throw std::runtime_error("Failed to get buffer device address");
    }
}

RessourceBuilder::RessourceBuilder(VkPhysicalDevice physicalDevice, VkDevice device, CommandManager commandManager) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->commandManager = commandManager;
}

AllocatedBuffer RessourceBuilder::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    AllocatedBuffer allocatedBuffer{};

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &allocatedBuffer.handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, allocatedBuffer.handle, &memRequirements);

    VkMemoryAllocateFlagsInfo allocateFlagsInfo{};
    allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    allocInfo.pNext = &allocateFlagsInfo;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &allocatedBuffer.bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, allocatedBuffer.handle, allocatedBuffer.bufferMemory, 0);

    VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
    buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    buffer_device_address_info.buffer = allocatedBuffer.handle;
    allocatedBuffer.deviceAddress = GetBufferDeviceAddressKHR(device, &buffer_device_address_info);

    return allocatedBuffer;
}

uint32_t RessourceBuilder::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperies;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperies);

    for (uint32_t i = 0; i < memProperies.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperies.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void RessourceBuilder::copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = commandManager.beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src.handle, dst.handle, 1, &copyRegion);

    commandManager.endSingleTimeCommand(commandBuffer);
}

void RessourceBuilder::destroyBuffer(AllocatedBuffer buffer) {
    vkDestroyBuffer(device, buffer.handle, nullptr);
    vkFreeMemory(device, buffer.bufferMemory, nullptr);
}

AllocatedImage RessourceBuilder::createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageLayout initialLayout,
                                             VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
    AllocatedImage image{};
    image.imageExtent = extent;
    image.imageFormat = format;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = initialLayout;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &image.image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image.imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate texture image memory!");
    }

    vkBindImageMemory(device, image.image, image.imageMemory, 0);

    image.imageView = createImageView(image.image, format, aspectFlags);

    return image;
}

AllocatedImage RessourceBuilder::createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling,
                                             VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
    createImage(extent, format, tiling, VK_IMAGE_LAYOUT_UNDEFINED, usage, aspectFlags);
}

AllocatedImage RessourceBuilder::createImage(void* data, VkExtent3D extent, VkFormat format, VkImageTiling tiling,
                                             VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
    VkDeviceSize imageSize = extent.width * extent.height * extent.depth;
    if (format == VK_FORMAT_R8G8B8_SRGB) {
        imageSize *= 3;
    } else if (format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_R8G8B8A8_UNORM){
        imageSize *= 4;
    } else {
        throw std::invalid_argument("Image format not supported!");
    }

    AllocatedBuffer stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* imageData;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, imageSize, 0, &imageData);
    memcpy(imageData, data, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    AllocatedImage image = createImage(extent, format, tiling, VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR | usage, aspectFlags);

    transitionImageLayout(image.image, format,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer.handle, image.image, extent);
    transitionImageLayout(image.image, format,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    destroyBuffer(stagingBuffer);
    return image;
}

void RessourceBuilder::transitionImageLayout(VkImage image, VkFormat format,
                                             VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = commandManager.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    commandManager.endSingleTimeCommand(commandBuffer);
}

void RessourceBuilder::copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D extent) {
    VkCommandBuffer commandBuffer = commandManager.beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    commandManager.endSingleTimeCommand(commandBuffer);
}

VkImageView RessourceBuilder::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;

    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void RessourceBuilder::destroyImage(AllocatedImage image) {
    vkDestroyImageView(device, image.imageView, nullptr);
    vkDestroyImage(device, image.image, nullptr);
    vkFreeMemory(device, image.imageMemory, nullptr);
}