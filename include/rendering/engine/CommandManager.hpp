#ifndef BASICS_COMMANDMANAGER_HPP
#define BASICS_COMMANDMANAGER_HPP

#include <DeviceManager.hpp>
#include <VulkanUtil.hpp>
#include <memory>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	class CommandManager {
	public:
		VkCommandPool commandPool{};

		CommandManager();
		CommandManager(std::shared_ptr<DeviceManager> deviceManager);
		void createCommandPool();
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommand(VkCommandBuffer commandBuffer);
		void destroyCommandManager();

	private:
		std::shared_ptr<DeviceManager> deviceManager;
	};

} // namespace RtEngine
#endif // BASICS_COMMANDMANAGER_HPP
