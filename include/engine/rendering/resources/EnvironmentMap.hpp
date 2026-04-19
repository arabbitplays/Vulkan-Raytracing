//
// Created by oschdi on 17.01.26.
//

#ifndef VULKAN_RAYTRACING_ENVIRONMENTMAP_HPP
#define VULKAN_RAYTRACING_ENVIRONMENTMAP_HPP
#include "DescriptorAllocator.hpp"
#include "TextureRepository.hpp"
#include <yaml-cpp/yaml.h>

namespace RtEngine {
    class EnvironmentMap {
    public:
        EnvironmentMap(std::shared_ptr<TextureRepository> tex_repo);

        void loadFromYaml(YAML::Node node);
        void writeToDescriptor(const std::shared_ptr<DescriptorAllocator> &descriptor_allocator, VkSampler sampler);
    private:
        std::vector<std::shared_ptr<Texture>> textures;

        std::shared_ptr<TextureRepository> tex_repo;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_ENVIRONMENTMAP_HPP