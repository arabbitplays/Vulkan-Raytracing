#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <BaseOptions.hpp>
#include <Swapchain.hpp>
#include <RessourceBuilder.hpp>
#include <MeshAssetBuilder.hpp>
#include <DescriptorAllocator.hpp>
#include <CommandManager.hpp>
#include <MeshRepository.hpp>
#include <TextureRepository.hpp>
#include <DeviceManager.hpp>

namespace RtEngine {
struct VulkanContext {
    std::shared_ptr<DeviceManager> device_manager;

    std::shared_ptr<Swapchain> swapchain;

    std::shared_ptr<RessourceBuilder> resource_builder;
    std::shared_ptr<DescriptorAllocator> descriptor_allocator;
    std::shared_ptr<CommandManager> command_manager;

    std::shared_ptr<TextureRepository> texture_repository;
    std::shared_ptr<MeshRepository> mesh_repository;

    std::shared_ptr<BaseOptions> base_options;
};

}
#endif //VULKANCONTEXT_HPP
