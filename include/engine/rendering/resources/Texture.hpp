#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vulkan/vulkan_core.h>

namespace RtEngine {
	struct AllocatedImage {
		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkFormat imageFormat;
		VkExtent3D imageExtent;
	};

	enum TextureType {
		NORMAL,
		PARAMETER,
		ENVIRONMENT,
	};

	class Texture {
	public:
		Texture() = default;
		Texture(std::string name, TextureType type, std::string path, AllocatedImage image) :
			name(name), type(type), path(path), image(image){};
		bool operator==(const Texture &other) const { return name == other.name; }

		std::string name;
		std::string path;
		TextureType type;
		AllocatedImage image;
	};

} // namespace RtEngine
#endif // TEXTURE_H
