//
// Created by oschdi on 3/23/25.
//

#include "TextureRepository.hpp"
#include <glm/glm.hpp>>
#include <spdlog/spdlog.h>

TextureRepository::TextureRepository(std::shared_ptr<RessourceBuilder> resource_builder) : resource_builder(resource_builder)
{
    initDefaultTextures();
}

void TextureRepository::initDefaultTextures()
{
    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    default_tex = std::make_shared<Texture>("", PARAMETER, "", resource_builder->createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT));

    uint32_t blue = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1, 0));
    default_normal_tex = std::make_shared<Texture>("", NORMAL, "", resource_builder->createImage((void*)&blue, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_UNORM,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT));

    deletion_queue.pushFunction([&]() {
        resource_builder->destroyImage(default_tex->image);
        resource_builder->destroyImage(default_normal_tex->image);
    });
}

std::shared_ptr<Texture> TextureRepository::getTexture(const std::string& name, const TextureType type)
{
    if (name.empty() || !texture_name_cache.contains(name))
    {
        return type == PARAMETER ? default_tex : default_normal_tex;
    } else
    {
        return texture_name_cache[name];
    }
}

void TextureRepository::addTexture(std::string path, TextureType type)
{
    if (texture_path_cache.contains(path))
    {
        spdlog::debug("Texture cache hit with path: {}", path);
        return;
    }

    Texture tex = resource_builder->loadTextureImage(path, type);
    texture_name_cache[tex.name] = std::make_shared<Texture>(tex);
    texture_path_cache[path] = texture_name_cache[tex.name];
}

void TextureRepository::destroy()
{
    deletion_queue.flush();

    for (auto& textures : texture_name_cache)
    {
        resource_builder->destroyImage(textures.second->image);
    }
}
