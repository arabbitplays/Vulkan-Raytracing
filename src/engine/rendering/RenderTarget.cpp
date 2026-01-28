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
                    VkExtent3D{image_extent.width, image_extent.height, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
                    VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);

            resource_builder->transitionImageLayout(
                    render_targets[i].image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }

        std::vector<uint32_t> pixels(image_extent.width * image_extent.height * 4);

        for (uint32_t i = 0; i < image_extent.width * image_extent.height * 4; i++) {
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

    void RenderTarget::recreate(const VkExtent2D new_image_extent)
    {
        this->image_extent = new_image_extent;
        uint32_t image_count = render_targets.size();
        destroy();
        createImages(image_count);
        resetAccumulatedFrames();
    }


    AllocatedImage RenderTarget::getCurrentTargetImage() const
    {
        return render_targets[current_image];
    }

    AllocatedImage RenderTarget::getLastTargetImage() const {
        uint32_t idx = current_image != 0 ? current_image - 1 : render_targets.size() - 1;
        return render_targets[idx];
    }


    AllocatedImage RenderTarget::getCurrentRngImage() const
    {
        return rng_textures[current_image];
    }

    void RenderTarget::nextImage()
    {
        current_image = (current_image + 1) % render_targets.size();
    }

    VkExtent2D RenderTarget::getExtent() const {
        return image_extent;
    }

    uint32_t RenderTarget::getAccumulatedFrameCount() const {
        return accumulated_frame_count;
    }

    void RenderTarget::resetAccumulatedFrames() {
        accumulated_frame_count = 0;
    }

    void RenderTarget::incrementAccumulatedFrameCount() {
        accumulated_frame_count++;
    }

    uint32_t RenderTarget::getTotalSampleCount() const {
        return accumulated_frame_count * samples_per_frame;
    }

    uint32_t RenderTarget::getSamplesPerFrame() const {
        return samples_per_frame;
    }

    void RenderTarget::setSamplesPerFrame(uint32_t new_samples_per_frame) {
        samples_per_frame = new_samples_per_frame;
    }

    void RenderTarget::destroy() const {
        for (auto &image: render_targets) {
            resource_builder->destroyImage(image);
        }

        for (auto &image: rng_textures) {
            resource_builder->destroyImage(image);
        }
    }


}
