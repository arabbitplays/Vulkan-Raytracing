//
// Created by oschdi on 12/30/24.
//

#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <vulkan/vulkan_core.h>
#include <Swapchain.hpp>
#include <RessourceBuilder.hpp>
#include <MeshAssetBuilder.hpp>
#include <DescriptorAllocator.hpp>
#include <CommandManager.hpp>
#include <TextureRepository.hpp>


struct VulkanContext {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkSurfaceKHR surface;

    std::shared_ptr<Swapchain> swapchain;

    std::shared_ptr<RessourceBuilder> resource_builder;
    std::shared_ptr<MeshAssetBuilder> mesh_builder;
    std::shared_ptr<DescriptorAllocator> descriptor_allocator;
    std::shared_ptr<CommandManager> command_manager;

    std::shared_ptr<TextureRepository> texture_repository;
};

#endif //VULKANCONTEXT_HPP
