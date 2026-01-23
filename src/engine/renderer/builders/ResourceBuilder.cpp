#include "ResourceBuilder.hpp"
#include <stdexcept>

#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <PathUtil.hpp>
#include <stb_image.h>
#include <stb_image_write.h>

#include <glm/vector_relational.hpp>
#include <spdlog/spdlog.h>

#include "QuickTimer.hpp"

namespace RtEngine {
	VkDeviceAddress GetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfoKHR *address_info) {
		auto func = (PFN_vkGetBufferDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
		if (func != nullptr) {
			return func(device, address_info);
		} else {
			throw std::runtime_error("Failed to get buffer device address");
		}
	}

	AllocatedBuffer ResourceBuilder::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
												  VkMemoryPropertyFlags properties) {
		VkDevice device = device_manager->getDevice();

		AllocatedBuffer allocatedBuffer{};
		allocatedBuffer.size = size;

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
		buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		buffer_device_address_info.buffer = allocatedBuffer.handle;
		allocatedBuffer.deviceAddress = GetBufferDeviceAddressKHR(device, &buffer_device_address_info);

		return allocatedBuffer;
	}

	AllocatedBuffer ResourceBuilder::stageMemoryToNewBuffer(void *data, size_t size, VkBufferUsageFlags usage) {
		VkDevice device = device_manager->getDevice();

		AllocatedBuffer stagingBuffer =
				createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void *mapped_data;
		vkMapMemory(device, stagingBuffer.bufferMemory, 0, size, 0, &mapped_data);
		memcpy(mapped_data, data, size);
		vkUnmapMemory(device, stagingBuffer.bufferMemory);

		AllocatedBuffer mapping_buffer =
				createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		copyBuffer(stagingBuffer, mapping_buffer, size);
		destroyBuffer(stagingBuffer);
		return mapping_buffer;
	}

	uint32_t ResourceBuilder::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperies;
		vkGetPhysicalDeviceMemoryProperties(device_manager->getPhysicalDevice(), &memProperies);

		for (uint32_t i = 0; i < memProperies.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProperies.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void ResourceBuilder::copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, VkDeviceSize size) {
		VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, src.handle, dst.handle, 1, &copyRegion);

		commandManager->endSingleTimeCommand(commandBuffer);
	}

	void ResourceBuilder::destroyBuffer(AllocatedBuffer buffer) {
		vkDestroyBuffer(device_manager->getDevice(), buffer.handle, nullptr);
		vkFreeMemory(device_manager->getDevice(), buffer.bufferMemory, nullptr);
	}

	AllocatedImage ResourceBuilder::createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling,
												VkImageUsageFlags usage, VkImageAspectFlags aspectFlags) {
		VkDevice device = device_manager->getDevice();

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
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &image.imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate texture image memory!");
		}

		vkBindImageMemory(device, image.image, image.imageMemory, 0);

		image.imageView = createImageView(image.image, format, aspectFlags);

		return image;
	}

	AllocatedImage ResourceBuilder::createImage(void *data, VkExtent3D extent, VkFormat format, VkImageTiling tiling,
												VkImageUsageFlags usage, VkImageAspectFlags aspectFlags,
												VkImageLayout target_layout) {
		VkDeviceSize imageSize = extent.width * extent.height * extent.depth;
		if (format == VK_FORMAT_R8G8B8_SRGB) {
			imageSize *= 3;
		} else if (format == VK_FORMAT_R8G8B8A8_SRGB || format == VK_FORMAT_R8G8B8A8_UNORM) {
			imageSize *= 4;
		} else if (format == VK_FORMAT_R32G32B32A32_UINT) {
			imageSize *= 16;
		} else {
			throw std::invalid_argument("Image format not supported!");
		}

		AllocatedBuffer stagingBuffer =
				createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDevice device = device_manager->getDevice();
		void *imageData;
		vkMapMemory(device, stagingBuffer.bufferMemory, 0, imageSize, 0, &imageData);
		memcpy(imageData, data, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBuffer.bufferMemory);

		AllocatedImage image =
				createImage(extent, format, tiling, VK_BUFFER_USAGE_2_TRANSFER_DST_BIT_KHR | usage, aspectFlags);

		transitionImageLayout(image.image, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							  VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copyBufferToImage(stagingBuffer.handle, image.image, extent);
		transitionImageLayout(image.image, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							  VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
							  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, target_layout);

		destroyBuffer(stagingBuffer);
		return image;
	}

	Texture ResourceBuilder::loadTextureImage(std::string path, TextureType type) {
		int texWidth, texHeight, texChannels;
		uint8_t *pixels = loadImageData(resource_path + "/" + path, &texWidth, &texHeight, &texChannels);

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkFormat format;
		if (type == NORMAL) {
			format = VK_FORMAT_R8G8B8A8_UNORM;
		} else if (type == PARAMETER) {
			format = VK_FORMAT_R8G8B8A8_SRGB;
		} else if (type == ENVIRONMENT) {
			format = VK_FORMAT_R8G8B8A8_SRGB;
		} else {
			SPDLOG_ERROR("Texture type not supported!");
		}

		AllocatedImage textureImage =
				createImage(pixels, {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}, format,
							VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		stbi_image_free(pixels);

		return Texture(PathUtil::getFileName(path), type, path, textureImage);
	}

	uint8_t *ResourceBuilder::loadImageData(std::string path, int *width, int *height, int *channels) {
		return stbi_load(path.c_str(), width, height, channels, STBI_rgb_alpha);
	}

	uint8_t *ResourceBuilder::downloadImage(AllocatedImage image, uint32_t bytes_per_channel) {
		QuickTimer timer("download image");

		size_t buffer_size =
				image.imageExtent.width * image.imageExtent.height * image.imageExtent.depth * 4 * bytes_per_channel;
		AllocatedBuffer staging_buffer =
				createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		transitionImageLayout(image.image, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
							  VK_ACCESS_NONE, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
							  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		copyImageToBuffer(image.image, staging_buffer.handle, image.imageExtent);
		transitionImageLayout(image.image, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
							  VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							  VK_IMAGE_LAYOUT_GENERAL);

		VkDevice device = device_manager->getDevice();
		void *mapped_data;
		auto *imageData = new uint8_t[buffer_size * bytes_per_channel];
		vkMapMemory(device, staging_buffer.bufferMemory, 0, buffer_size, 0, &mapped_data);
		memcpy(imageData, mapped_data, buffer_size);
		vkUnmapMemory(device, staging_buffer.bufferMemory);

		destroyBuffer(staging_buffer);

		return imageData;
	}

	void ResourceBuilder::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
												VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
												VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
												VkImageLayout oldLayout, VkImageLayout newLayout) {

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
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;

		vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void ResourceBuilder::transitionImageLayout(VkImage image, VkPipelineStageFlags srcStage,
												VkPipelineStageFlags dstStage, VkAccessFlags srcAccessMask,
												VkAccessFlags dstAccessMask, VkImageLayout oldLayout,
												VkImageLayout newLayout) {

		VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
		transitionImageLayout(commandBuffer, image, srcStage, dstStage, srcAccessMask, dstAccessMask, oldLayout,
							  newLayout);
		commandManager->endSingleTimeCommand(commandBuffer);
	}

	void ResourceBuilder::copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D extent) {
		VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();

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

		commandManager->endSingleTimeCommand(commandBuffer);
	}

	void ResourceBuilder::copyImageToBuffer(VkImage image, VkBuffer buffer, VkExtent3D extent) {
		VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();

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

		vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

		commandManager->endSingleTimeCommand(commandBuffer);
	}

	VkImageView ResourceBuilder::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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
		if (vkCreateImageView(device_manager->getDevice(), &createInfo, nullptr, &imageView) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view!");
		}

		return imageView;
	}

	void ResourceBuilder::destroyImage(AllocatedImage image) {
		VkDevice device = device_manager->getDevice();
		vkDestroyImageView(device, image.imageView, nullptr);
		vkDestroyImage(device, image.image, nullptr);
		vkFreeMemory(device, image.imageMemory, nullptr);
	}
} // namespace RtEngine
