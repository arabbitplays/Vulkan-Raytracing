#ifndef TEXTUREREPOSITORY_HPP
#define TEXTUREREPOSITORY_HPP

#include <DeletionQueue.hpp>
#include <ResourceBuilder.hpp>
#include <memory>
#include <unordered_map>
#include <spdlog/spdlog.h>

namespace RtEngine {
	template <uint32_t N = 64>
	class TextureRepository {
	public:
		static constexpr uint32_t MAX_TEXTURE_COUNT = N;

		TextureRepository() = default;
		TextureRepository(std::shared_ptr<ResourceBuilder> resource_builder) : resource_builder(resource_builder) {
			initDefaultTextures();
		}

		std::vector<std::shared_ptr<Texture>> getNonDefaultTextures() {
			std::vector<std::shared_ptr<Texture>> result{};
			for (uint32_t i = 0; i < next_tex_idx; i++) {
				std::shared_ptr<Texture> tex = ordered_textures[i];
				if (tex->name != default_tex->name && tex->name != default_normal_tex->name) {
					result.push_back(tex);
				}
			}
			return result;
		}

		std::vector<VkImageView> getOrderedImageViews() {
			std::vector<VkImageView> image_views(MAX_TEXTURE_COUNT);
			for (uint32_t i = 0; i < next_tex_idx; i++) {
				image_views[i] = ordered_textures[i]->image.imageView;
			}

			for (uint32_t i = next_tex_idx; i < MAX_TEXTURE_COUNT; i++) {
				image_views[i] = default_tex->image.imageView;
			}
			return image_views;
		}

		int32_t getTextureIndex(const std::string &name) {
			if (texture_name_cache.contains(name)) {
				return texture_name_cache[name];
			}
			return -1;
		}

		std::shared_ptr<Texture> addTexture(std::string path, TextureType type) {
			if (texture_path_cache.contains(path)) {
				spdlog::debug(fmt::runtime("Texture cache hit with path: {}"), path);
				return ordered_textures[texture_path_cache[path]];
			}

			const std::shared_ptr<Texture> tex = std::make_shared<Texture>(resource_builder->loadTextureImage(path, type));
			return addTexture(tex);
		}

		std::shared_ptr<Texture> addTexture(const std::shared_ptr<Texture>& tex) {
			ordered_textures[next_tex_idx] = tex;
			texture_name_cache[tex->name] = next_tex_idx;
			texture_path_cache[tex->path] = next_tex_idx;
			next_tex_idx++;
			return tex;
		}

		std::shared_ptr<Texture> getDefaultTex(const TextureType type) {
			return type == PARAMETER ? default_tex : default_normal_tex;
		}

		void clear() {
			for (uint32_t i = 0; i < next_tex_idx; i++) {
				resource_builder->destroyImage(ordered_textures[i]->image);
			}
			next_tex_idx = 0;

			initDefaultTextures();
		}

		void destroy() {
			deletion_queue.flush();
			for (uint32_t i = 0; i < next_tex_idx; i++) {
				resource_builder->destroyImage(ordered_textures[i]->image);
			}
		}

	private:
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

		std::shared_ptr<ResourceBuilder> resource_builder;
		DeletionQueue deletion_queue;

		// these are ordered so that the texture indices given out to the
		std::array<std::shared_ptr<Texture>, MAX_TEXTURE_COUNT> ordered_textures{};
		uint32_t next_tex_idx = 0;

		std::unordered_map<std::string, uint32_t> texture_name_cache, texture_path_cache;
		std::shared_ptr<Texture> default_tex, default_normal_tex;
	};

} // namespace RtEngine
#endif // TEXTUREREPOSITORY_HPP
