//
// Created by oschdi on 17.01.26.
//

#include "../../../include/engine/resources/EnvironmentMap.hpp"

#include "DescriptorAllocator.hpp"

namespace RtEngine {
    EnvironmentMap::EnvironmentMap(std::shared_ptr<TextureRepository> tex_repo) : tex_repo(tex_repo) {

    }

    void EnvironmentMap::writeToDescriptor(const std::shared_ptr<DescriptorAllocator> &descriptor_allocator, const VkSampler sampler) {
        std::vector<VkImageView> views{};
        for (uint32_t i = 0; i < textures.size(); i++) {
            views.push_back(textures[i]->image.imageView);
        }

        for (uint32_t i = views.size(); i < 6; i++) {
            views.push_back(tex_repo->getDefaultTex(ENVIRONMENT)->image.imageView);
        }

        descriptor_allocator->writeImages(8, views, sampler,
                                                          VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    void EnvironmentMap::loadFromYaml(YAML::Node node) {
        for (const auto &texture_node: node["textures"]) {
            std::string path = texture_node.as<std::string>();
            textures.push_back(tex_repo->addTexture(path, ENVIRONMENT));
        }
    }
} // RtEngine