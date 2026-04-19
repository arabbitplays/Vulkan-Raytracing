#ifndef VULKAN_RAYTRACING_RNGTARGET_HPP
#define VULKAN_RAYTRACING_RNGTARGET_HPP
#include "RenderTarget.hpp"

namespace RtEngine {
    class RngTarget : public RenderTarget {
    public:
        RngTarget(const std::shared_ptr<ResourceBuilder> &resource_builder, uint32_t max_frames_in_flight);

        void createTargetImages(VkExtent2D image_extent) override;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RNGTARGET_HPP