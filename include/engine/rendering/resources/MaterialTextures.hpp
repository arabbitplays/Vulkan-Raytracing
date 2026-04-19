#ifndef TEXTUREREPOSITORY_HPP
#define TEXTUREREPOSITORY_HPP

#include <DeletionQueue.hpp>
#include <ResourceBuilder.hpp>
#include <memory>
#include <unordered_map>
#include <spdlog/spdlog.h>

#include "TextureRepository.hpp"

namespace RtEngine {
	template <uint32_t N = 64>
	class MaterialTextures {
	public:
		static constexpr uint32_t MAX_TEXTURE_COUNT = N;

		MaterialTextures() = default;
		MaterialTextures(std::shared_ptr<TextureRepository> texture_repository) : texture_repository(texture_repository) {
		}

		std::vector<VkImageView> getOrderedImageViews() {
			std::vector<VkImageView> image_views(MAX_TEXTURE_COUNT);
			for (uint32_t i = 0; i < next_tex_idx; i++) {
				image_views[i] = ordered_textures[i]->image.imageView;
			}

			for (uint32_t i = next_tex_idx; i < MAX_TEXTURE_COUNT; i++) {
				image_views[i] = texture_repository->getDefaultTex(PARAMETER)->image.imageView;
			}
			return image_views;
		}

		uint32_t addTexture(const std::shared_ptr<Texture>& tex) {

			if (texture_name_cache.contains(tex->name)) {
				return texture_name_cache[tex->name];
			}

			ordered_textures[next_tex_idx] = tex;
			texture_name_cache[tex->name] = next_tex_idx;
			next_tex_idx++;
			return next_tex_idx - 1;
		}

		void clear() {
			next_tex_idx = 0;
			texture_name_cache.clear();
		}

	private:


		std::shared_ptr<TextureRepository> texture_repository;

		// these are ordered so that the texture indices given out to the
		std::array<std::shared_ptr<Texture>, MAX_TEXTURE_COUNT> ordered_textures{};
		uint32_t next_tex_idx = 0;

		std::unordered_map<std::string, uint32_t> texture_name_cache{};
	};

} // namespace RtEngine
#endif // TEXTUREREPOSITORY_HPP
