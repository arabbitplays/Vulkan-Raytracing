//
// Created by oschdi on 17.01.26.
//

#ifndef VULKAN_RAYTRACING_TEXTUREREPOSITORY_HPP
#define VULKAN_RAYTRACING_TEXTUREREPOSITORY_HPP

#include <glm/packing.hpp>
#include <glm/vec4.hpp>
#include <spdlog/spdlog.h>

#include "ResourceBuilder.hpp"
#include "Texture.hpp"

namespace RtEngine {
    class TextureRepository {
    public:
        TextureRepository(std::shared_ptr<ResourceBuilder> resource_builder) : resource_builder(resource_builder) {
            initDefaultTextures();
        }

        std::shared_ptr<Texture> addTexture(std::string path, TextureType type) {
            if (texture_path_cache.contains(path)) {
                spdlog::debug(fmt::runtime("Texture cache hit with path: {}"), path);
                return texture_path_cache[path];
            }

            const std::shared_ptr<Texture> tex = std::make_shared<Texture>(resource_builder->loadTextureImage(path, type));
            texture_name_cache[tex->name] = tex;
            texture_path_cache[tex->path] = tex;
            return tex;
        }

        std::shared_ptr<Texture> getTextureByName(const std::string& name) {
            assert(texture_name_cache.contains(name));
            return texture_name_cache[name];
        }

        void initDefaultTextures() {
            uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
            default_tex = std::make_shared<Texture>(
                    "def_prop", PARAMETER, "",
                    resource_builder->createImage((void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                                                  VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT,
                                                  VK_IMAGE_ASPECT_COLOR_BIT));

            uint32_t blue = glm::packUnorm4x8(glm::vec4(0.5f, 0.5f, 1, 0));
            default_normal_tex = std::make_shared<Texture>(
                    "def_normal", NORMAL, "",
                    resource_builder->createImage((void *) &blue, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM,
                                                  VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT,
                                                  VK_IMAGE_ASPECT_COLOR_BIT));

            addTexture(default_tex);
            addTexture(default_normal_tex);
        }

        std::shared_ptr<Texture> getDefaultTex(const TextureType type) {
            return type == PARAMETER ? default_tex : default_normal_tex;
        }

        void destroy() {
            for (const auto&[_, tex] : texture_name_cache) {
                resource_builder->destroyImage(tex->image);
            }
            texture_name_cache.clear();
            texture_path_cache.clear();
        }

    private:
        std::shared_ptr<Texture> addTexture(std::shared_ptr<Texture> tex) {
            texture_name_cache[tex->name] = tex;
            texture_path_cache[tex->path] = tex;
            return tex;
        }

        std::shared_ptr<ResourceBuilder> resource_builder;
        std::unordered_map<std::string, std::shared_ptr<Texture>> texture_name_cache, texture_path_cache;

        std::shared_ptr<Texture> default_tex, default_normal_tex;


    };
} // RtEngine

#endif //VULKAN_RAYTRACING_TEXTUREREPOSITORY_HPP