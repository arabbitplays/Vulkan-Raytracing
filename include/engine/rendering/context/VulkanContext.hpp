#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <CommandManager.hpp>
#include <DescriptorAllocator.hpp>
#include <DeviceManager.hpp>
#include <MeshAssetBuilder.hpp>
#include <ResourceBuilder.hpp>
#include <Swapchain.hpp>

#include "../../Window.hpp"

namespace RtEngine {
	struct VulkanContext {
		std::shared_ptr<DeviceManager> device_manager;
		std::shared_ptr<Window> window;
		std::shared_ptr<Swapchain> swapchain;

		std::shared_ptr<ResourceBuilder> resource_builder;
		std::shared_ptr<DescriptorAllocator> descriptor_allocator;
		std::shared_ptr<CommandManager> command_manager;
	};

} // namespace RtEngine
#endif // VULKANCONTEXT_HPP
