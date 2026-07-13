#include "targets/SampleCountTarget.hpp"

namespace RtEngine {
    void SampleCountTarget::createTargetImages(VkExtent2D image_extent) {
        target_images.resize(max_frames_in_flight);
        for (uint32_t i = 0; i < max_frames_in_flight; i++) {
            target_images[i] = resource_builder->createImage(
                VkExtent3D{image_extent.width, image_extent.height, 1}, VK_FORMAT_R32_UINT,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
} // RtEngine
