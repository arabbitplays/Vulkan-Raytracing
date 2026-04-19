#ifndef VULKAN_RAYTRACING_RENDERTARGET_HPP
#define VULKAN_RAYTRACING_RENDERTARGET_HPP
#include "ResourceBuilder.hpp"

namespace RtEngine {
    class RenderTarget {
    public:
        virtual ~RenderTarget() = default;

        RenderTarget(const std::shared_ptr<ResourceBuilder> &resource_builder, uint32_t max_frames_in_flight);

        virtual void createTargetImages(VkExtent2D image_extent) = 0;
        void recreate(VkExtent2D image_extent);

        AllocatedImage getImage(uint32_t idx);

        void destroy();


    protected:
        std::shared_ptr<ResourceBuilder> resource_builder;
        uint32_t max_frames_in_flight;

        std::vector<AllocatedImage> target_images{};
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RENDERTARGET_HPP