//
// Created by oschdi on 6/6/25.
//

#include "RenderTarget.hpp"

#include <RandomUtil.hpp>

namespace RtEngine
{
    RenderTarget::RenderTarget(const std::shared_ptr<ResourceBuilder>& resource_builder, VkExtent2D image_extent, uint32_t max_frames_in_flight)
        : resource_builder(resource_builder), image_extent(image_extent)
    {
        createImages(max_frames_in_flight);
    };

    void RenderTarget::createImages(uint32_t image_count) {
        render_targets.resize(image_count);
        for (uint32_t i = 0; i < image_count; i++) {
            render_targets[i] = resource_builder->createImage(
                    VkExtent3D{image_extent.width, image_extent.height, 1}, VK_FORMAT_R8G8B8A8_UNORM, // TODO read out needed format from the swap chain
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);

            resource_builder->transitionImageLayout(
                    render_targets[i].image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }

        std::vector<uint32_t> pixels(image_extent.width * image_extent.height * 4);

        for (int i = 0; i < image_extent.width * image_extent.height * 4; i++) {
            pixels[i] = RandomUtil::generateInt();
        }

        rng_textures.resize(image_count);
        for (uint32_t i = 0; i < image_count; i++) {
            rng_textures[i] = resource_builder->createImage(
                    pixels.data(), VkExtent3D{image_extent.width, image_extent.height, 1},
                    VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL);
        }
    }

    void RenderTarget::recreate(VkExtent2D image_extent)
    {
        this->image_extent = image_extent;
        uint32_t image_count = render_targets.size();
        destroy();
        createImages(image_count);
    }


    AllocatedImage RenderTarget::getCurrentTargetImage() const
    {
        return render_targets[current_image];
    }


    AllocatedImage RenderTarget::getCurrentRngImage() const
    {
        return rng_textures[current_image];
    }

    void RenderTarget::nextImage()
    {
        current_image = (current_image + 1) % render_targets.size();
    }


    void RenderTarget::destroy()
    {
        for (auto &image: render_targets) {
            resource_builder->destroyImage(image);
        }

        for (auto &image: rng_textures) {
            resource_builder->destroyImage(image);
        }
    }


}
