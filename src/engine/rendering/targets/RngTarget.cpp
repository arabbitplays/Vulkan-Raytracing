#include "targets/RngTarget.hpp"

#include "RandomUtil.hpp"

namespace RtEngine {
    RngTarget::RngTarget(const std::shared_ptr<ResourceBuilder> &resource_builder,
                         uint32_t max_frames_in_flight) : RenderTarget(resource_builder, max_frames_in_flight) {
    }

    void RngTarget::createTargetImages(VkExtent2D image_extent) {
        std::vector<uint32_t> pixels(image_extent.width * image_extent.height * 4);
        for (uint32_t i = 0; i < image_extent.width * image_extent.height * 4; i++) {
            pixels[i] = RandomUtil::generateInt();
        }

        target_images.resize(max_frames_in_flight);
        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            target_images[i] = resource_builder->createImage(
                pixels.data(), VkExtent3D{image_extent.width, image_extent.height, 1},
                VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
} // RtEngine
