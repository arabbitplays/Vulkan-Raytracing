#include "CommandManager.hpp"
#include <stdexcept>

namespace RtEngine {
	CommandManager::CommandManager() = default;

	CommandManager::CommandManager(const std::shared_ptr<DeviceManager> &deviceManager) : deviceManager(deviceManager) {
		createCommandPool();
	}

	void CommandManager::createCommandPool() {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = deviceManager->getQueueIndices().graphicsAndComputeFamily.value();

		if (vkCreateCommandPool(deviceManager->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	VkCommandBuffer CommandManager::beginSingleTimeCommands() const {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(deviceManager->getDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void CommandManager::endSingleTimeCommand(const VkCommandBuffer commandBuffer) const {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkQueue graphics_queue = deviceManager->getQueue(GRAPHICS);
		vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics_queue);

		vkFreeCommandBuffers(deviceManager->getDevice(), commandPool, 1, &commandBuffer);
	}

	void CommandManager::destroy() const {
		vkDestroyCommandPool(deviceManager->getDevice(), commandPool, nullptr);
	}
} // namespace RtEngine
