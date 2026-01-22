#ifndef BASICS_RESSOURCEBUILDER_HPP
#define BASICS_RESSOURCEBUILDER_HPP

#include <Texture.hpp>
#include <cstring>
#include <stb_image.h>
#include <string>
#include <vulkan/vulkan_core.h>
#include "CommandManager.hpp"

namespace RtEngine {
	struct AllocatedBuffer {
		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceMemory bufferMemory;
		uint64_t deviceAddress;
		size_t size;

		void update(VkDevice device, void *data, size_t size, size_t offset = 0) {
			void *mapped_data;
			vkMapMemory(device, bufferMemory, offset, size, 0, &mapped_data);
			memcpy(mapped_data, data, size);
			vkUnmapMemory(device, bufferMemory);
		}
	};

	class ResourceBuilder {
	public:
		ResourceBuilder() = default;
		ResourceBuilder(std::shared_ptr<DeviceManager> device_manager, std::shared_ptr<CommandManager> commandManager,
						const std::string &resource_path) :
			device_manager(device_manager), commandManager(commandManager), resource_path(resource_path){};

		AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		AllocatedBuffer stageMemoryToNewBuffer(void *data, size_t size, VkBufferUsageFlags usage);
		void copyBuffer(AllocatedBuffer src, AllocatedBuffer dst, VkDeviceSize size);
		void destroyBuffer(AllocatedBuffer buffer);

		AllocatedImage createImage(VkExtent3D extent, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
								   VkImageAspectFlags aspectFlags);
		AllocatedImage createImage(void *data, VkExtent3D extent, VkFormat format, VkImageTiling tiling,
								   VkImageUsageFlags usage, VkImageAspectFlags aspectFlags,
								   VkImageLayout target_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		Texture loadTextureImage(std::string path, TextureType type = PARAMETER);
		uint8_t *loadImageData(std::string path, int *width, int *height, int *channels);
		void *downloadImage(AllocatedImage image, uint32_t bytes_per_channel = 1);

		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkPipelineStageFlags srcStage,
								   VkPipelineStageFlags dstStage, VkAccessFlags srcAccessMask,
								   VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);
		void transitionImageLayout(VkImage image, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
								   VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout,
								   VkImageLayout newLayout);

		void destroyImage(AllocatedImage image);

	private:
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		std::shared_ptr<DeviceManager> device_manager;
		std::shared_ptr<CommandManager> commandManager;
		std::string resource_path;

		void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D extent);
		void copyImageToBuffer(VkImage image, VkBuffer buffer, VkExtent3D extent);
	};

} // namespace RtEngine
#endif // BASICS_RESSOURCEBUILDER_HPP
