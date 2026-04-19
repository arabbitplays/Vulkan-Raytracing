#ifndef VULKAN_RAYTRACING_COMPUTETARGET_HPP
#define VULKAN_RAYTRACING_COMPUTETARGET_HPP
#include "RenderTarget.hpp"

namespace RtEngine {
    class ComputeTarget : public RenderTarget {
    public:
        ComputeTarget(const std::shared_ptr<ResourceBuilder> &resource_builder, uint32_t max_frames_in_flight)
            : RenderTarget(resource_builder, max_frames_in_flight) {
        }

        void createTargetImages(VkExtent2D image_extent) override;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTETARGET_HPP