//
// Created by oschdi on 2/8/25.
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include <vulkan/vulkan_core.h>
#include <string>

struct AllocatedImage {
  VkImage image;
  VkDeviceMemory imageMemory;
  VkImageView imageView;
  VkFormat imageFormat;
  VkExtent3D imageExtent;
};

enum TextureType
{
  NORMAL,
  PARAMETER,
};

class Texture {
public:
  Texture() = default;
  Texture(std::string name, TextureType type, std::string path, AllocatedImage image) : name(name), type(type), path(path), image(image) {};

  std::string name;
  std::string path;
  TextureType type;
  AllocatedImage image;
};



#endif //TEXTURE_H
