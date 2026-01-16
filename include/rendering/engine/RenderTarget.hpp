//
// Created by oschdi on 6/6/25.
//

#ifndef RENDERTARGET_HPP
#define RENDERTARGET_HPP
#include <memory>
#include <ResourceBuilder.hpp>
#include <Texture.hpp>
#include <vector>

namespace RtEngine
{
    class RenderTarget {
    public:
        RenderTarget() = default;
        explicit RenderTarget(const std::shared_ptr<ResourceBuilder>& resource_builder, VkExtent2D image_extent, uint32_t max_frames_in_flight);

        AllocatedImage getCurrentTargetImage() const;
        AllocatedImage getCurrentRngImage() const;
        void nextImage();

        uint32_t getCurrentSampleCount() const;

        void recreate(VkExtent2D image_extent);

        void destroy();

    private:
        void createImages(uint32_t image_count);

        std::shared_ptr<ResourceBuilder> resource_builder;

        VkExtent2D image_extent;

        uint32_t current_image = 0;
        std::vector<AllocatedImage> render_targets;
        std::vector<AllocatedImage> rng_textures;
    };
}



#endif //RENDERTARGET_HPP
